#include "code_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const char *name; int offset; } Local;

static FILE *out;
static Local locals[256];
static int   local_count;
static int   stack_size;
static int   label_id;

static int lookup_local(const char *name) {
    for (int i = local_count - 1; i >= 0; i--)
        if (strcmp(locals[i].name, name) == 0)
            return locals[i].offset;
    return 0;
}

static int add_local(const char *name) {
    int off = stack_size + 4;
    stack_size += 4;
    locals[local_count++] = (Local){ name, -off };
    return -off;
}

static void collect_locals(Node *node) {
    if (!node) return;
    switch (node->kind) {
    case ND_BLOCK:
        for (int i = 0; i < node->stmt_count; i++)
            collect_locals(node->stmts[i]);
        break;
    case ND_DECL:
        add_local(node->name);
        if (node->init)
            collect_locals(node->init);
        break;
    case ND_FOR:
        if (node->init) collect_locals(node->init);
        if (node->cond) collect_locals(node->cond);
        if (node->inc)  collect_locals(node->inc);
        if (node->rhs)  collect_locals(node->rhs);
        break;
    case ND_IF:
        if (node->lhs) collect_locals(node->lhs);
        if (node->rhs) collect_locals(node->rhs);
        if (node->els) collect_locals(node->els);
        break;
    case ND_WHILE:
        if (node->lhs) collect_locals(node->lhs);
        if (node->rhs) collect_locals(node->rhs);
        break;
    default:
        if (node->lhs) collect_locals(node->lhs);
        if (node->rhs) collect_locals(node->rhs);
        break;
    }
}

static void gen_addr(Node *node);
static void gen_expr(Node *node);
static void gen_stmt(Node *node, const char *ret_label);

static void gen_addr(Node *node) {
    switch (node->kind) {
    case ND_VAR: {
        int off = lookup_local(node->name);
        if (off) {
            fprintf(out, "    add r0, fp, #%d\n", off);
        } else {
            fprintf(out, "    ldr r0, =%s\n", node->name);
        }
        break;
    }
    case ND_DEREF:
        gen_expr(node->lhs);
        break;
    case ND_ADDR:
        gen_addr(node->lhs);
        break;
    default:
        break;
    }
}

static void gen_expr(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        fprintf(out, "    mov r0, #%d\n", node->val);
        break;
    case ND_VAR:
        gen_addr(node);
        fprintf(out, "    ldr r0, [r0]\n");
        break;
    case ND_ADDR:
        gen_addr(node->lhs);
        break;
    case ND_DEREF:
        gen_expr(node->lhs);
        fprintf(out, "    ldr r0, [r0]\n");
        break;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        fprintf(out, "    push {r0}\n");
        gen_expr(node->rhs);
        fprintf(out, "    pop {r1}\n");
        fprintf(out, "    str r0, [r1]\n");
        break;
    case ND_ADD:
        gen_expr(node->lhs);
        fprintf(out, "    push {r0}\n");
        gen_expr(node->rhs);
        fprintf(out, "    pop {r1}\n");
        fprintf(out, "    add r0, r1, r0\n");
        break;
    case ND_SUB:
        gen_expr(node->lhs);
        fprintf(out, "    push {r0}\n");
        gen_expr(node->rhs);
        fprintf(out, "    pop {r1}\n");
        fprintf(out, "    sub r0, r1, r0\n");
        break;
    case ND_MUL:
        gen_expr(node->lhs);
        fprintf(out, "    push {r0}\n");
        gen_expr(node->rhs);
        fprintf(out, "    pop {r1}\n");
        fprintf(out, "    mul r0, r1, r0\n");
        break;
    case ND_DIV:
        gen_expr(node->lhs);
        fprintf(out, "    push {r0}\n");
        gen_expr(node->rhs);
        fprintf(out, "    pop {r1}\n");
        fprintf(out, "    mov r2, r0\n");
        fprintf(out, "    mov r0, r1\n");
        fprintf(out, "    mov r1, r2\n");
        fprintf(out, "    bl __aeabi_idiv\n");
        break;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE: {
        gen_expr(node->lhs);
        fprintf(out, "    push {r0}\n");
        gen_expr(node->rhs);
        fprintf(out, "    pop {r1}\n");
        fprintf(out, "    cmp r1, r0\n");
        const char *cc = (node->kind==ND_EQ)?"eq":(node->kind==ND_NE)?"ne":(node->kind==ND_LT)?"lt":"le";
        fprintf(out, "    mov r0, #0\n");
        fprintf(out, "    mov%s r0, #1\n", cc);
        break;
    }
    case ND_CALL:
        /* NEW – avalia direita→esquerda; r0 já serve para o arg0*/
        for (int i = node->arg_count - 1; i >= 0 && i < 4; i--) {
            gen_expr(node->args[i]);              /* resultado em r0*/
            if (i != 0)                           /* evita mov r0,r0*/
                fprintf(out, "    mov r%d, r0\n", i);
        }
         fprintf(out, "    bl %s\n", node->name);
         break;
    case ND_POSTINC:
        gen_addr(node->lhs);
        fprintf(out, "    push {r0}\n");
        fprintf(out, "    ldr r0, [r0]\n");
        fprintf(out, "    mov r1, r0\n");
        fprintf(out, "    add r0, r0, #1\n");
        fprintf(out, "    pop {r2}\n");
        fprintf(out, "    str r0, [r2]\n");
        fprintf(out, "    mov r0, r1\n");
        break;
    case ND_POSTDEC:
        gen_addr(node->lhs);
        fprintf(out, "    push {r0}\n");
        fprintf(out, "    ldr r0, [r0]\n");
        fprintf(out, "    mov r1, r0\n");
        fprintf(out, "    sub r0, r0, #1\n");
        fprintf(out, "    pop {r2}\n");
        fprintf(out, "    str r0, [r2]\n");
        fprintf(out, "    mov r0, r1\n");
        break;
    default:
        break;
    }
}

static void gen_stmt(Node *node, const char *ret_label) {
    switch (node->kind) {
    case ND_RETURN:
        if (node->lhs) gen_expr(node->lhs);
        fprintf(out, "    b %s\n", ret_label);
        break;
    case ND_BLOCK:
        for (int i = 0; i < node->stmt_count; i++)
            gen_stmt(node->stmts[i], ret_label);
        break;
    case ND_IF: {
        int id = label_id++;
        char lelse[32];
        char lend[32];
        snprintf(lelse, sizeof lelse, ".Lelse%d", id);
        snprintf(lend, sizeof lend, ".Lend%d", id);
        gen_expr(node->lhs);
        fprintf(out, "    cmp r0, #0\n");
        if (node->els) {
            fprintf(out, "    beq %s\n", lelse);
            gen_stmt(node->rhs, ret_label);
            fprintf(out, "    b %s\n", lend);
            fprintf(out, "%s:\n", lelse);
            gen_stmt(node->els, ret_label);
            fprintf(out, "%s:\n", lend);
        } else {
            fprintf(out, "    beq %s\n", lend);
            gen_stmt(node->rhs, ret_label);
            fprintf(out, "%s:\n", lend);
        }
        break;
    }
    case ND_WHILE: {
        int id = label_id++;
        char lbegin[32];
        char lend[32];
        snprintf(lbegin, sizeof lbegin, ".Lbegin%d", id);
        snprintf(lend, sizeof lend, ".Lendw%d", id);
        fprintf(out, "%s:\n", lbegin);
        gen_expr(node->lhs);
        fprintf(out, "    cmp r0, #0\n");
        fprintf(out, "    beq %s\n", lend);
        gen_stmt(node->rhs, ret_label);
        fprintf(out, "    b %s\n", lbegin);
        fprintf(out, "%s:\n", lend);
        break;
    }
    case ND_FOR: {
        int id = label_id++;
        char lbegin[32];
        char lend[32];
        snprintf(lbegin, sizeof lbegin, ".Lfor%d", id);
        snprintf(lend, sizeof lend, ".Lendf%d", id);
        if (node->init) gen_stmt(node->init, ret_label);
        fprintf(out, "%s:\n", lbegin);
        if (node->cond) {
            gen_expr(node->cond);
            fprintf(out, "    cmp r0, #0\n");
            fprintf(out, "    beq %s\n", lend);
        }
        gen_stmt(node->rhs, ret_label);
        if (node->inc) gen_expr(node->inc);
        fprintf(out, "    b %s\n", lbegin);
        fprintf(out, "%s:\n", lend);
        break;
    }
    case ND_DECL: {
        int off = lookup_local(node->name);
        if (node->init) {
            gen_expr(node->init);
            fprintf(out, "    str r0, [fp, #%d]\n", off);
        }
        break;
    }
    default:
        gen_expr(node);
        break;
    }
}

static void gen_function(Node *fn) {
    local_count = 0;
    stack_size  = 0;
    for (int i = 0; i < fn->arg_count; i++)
        add_local(fn->args[i]->name);
    for (int i = 0; i < fn->stmt_count; i++)
        collect_locals(fn->stmts[i]);
    if (stack_size % 4)
        stack_size = (stack_size + 3) & ~3;

    fprintf(out, ".global %s\n", fn->name);
    fprintf(out, "%s:\n", fn->name);
    fprintf(out, "    push {fp, lr}\n");
    fprintf(out, "    mov fp, sp\n");
    if (stack_size)
        fprintf(out, "    sub sp, sp, #%d\n", stack_size);
    for (int i = 0; i < fn->arg_count && i < 4; i++) {
        int off = lookup_local(fn->args[i]->name);
        fprintf(out, "    str r%d, [fp, #%d]\n", i, off);
    }

    char epilogue[32], fallthrough[32];
    snprintf(epilogue,   sizeof epilogue,   ".Lep_%s",   fn->name);
    snprintf(fallthrough,sizeof fallthrough,".Lftr_%s",  fn->name);
    /* percorre corpo – passa ambos os rótulos               */
    for (int i = 0; i < fn->stmt_count; i++)
        gen_stmt(fn->stmts[i], epilogue);
    /* ----- queda no fim: r0 := 0 -------------------------- */
    fprintf(out, "%s:\n", fallthrough);
    fprintf(out, "    mov r0, #0\n");
    fprintf(out, "    b %s\n", epilogue);

    /* ----- epílogo comum ---------------------------------- */
    fprintf(out, "%s:\n", epilogue);
    fprintf(out, "    mov sp, fp\n");
    fprintf(out, "    pop {fp, pc}\n");
}


static void gen_global(Node *g) {
    if (g->init && g->init->kind == ND_NUM) {
        fprintf(out, "%s:\n    .word %d\n", g->name, g->init->val);
    } else {
        fprintf(out, "%s:\n    .word 0\n", g->name);
    }
}

void codegen_to_file(Node *root, const char *out_path) {
    out = fopen(out_path, "w");
    if (!out) {
        perror(out_path);
        return;
    }
    /* ---------- _start: chama main e finaliza via semihosting ----- */
    fprintf(out,
        ".text\n"
        ".global _start\n"
        "_start:\n"
        "    ldr sp, =_stack_top   @ pilha = topo reservado no linker\n"
        "    bl  main          @ chama main()\n"
        "    mov r7, #0x18     @ SYS_EXIT\n"
        "    svc 0x123456 \n\n");
        // "    swi 0xbb \n\n");

    /* globals */
    int has_glob = 0;
    for (int i = 0; i < root->stmt_count; i++)
        if (root->stmts[i]->kind == ND_DECL)
            has_glob = 1;
    if (has_glob) {
        fprintf(out, ".data\n");
        for (int i = 0; i < root->stmt_count; i++)
            if (root->stmts[i]->kind == ND_DECL)
                gen_global(root->stmts[i]);
    }

    fprintf(out, ".text\n");
    for (int i = 0; i < root->stmt_count; i++)
        if (root->stmts[i]->kind == ND_FUNC)
            gen_function(root->stmts[i]);

    fclose(out);
}
