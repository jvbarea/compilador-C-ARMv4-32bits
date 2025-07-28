#include "code_generator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================================
 * Estruturas auxiliares para mapeamento de variáveis locais
 * ======================================================================= */

typedef struct CGVar {
    const char *name;
    int offset;               /* deslocamento negativo a partir de fp */
    struct CGVar *next;
} CGVar;

typedef struct CGScope {
    CGVar *vars;
    struct CGScope *parent;
} CGScope;

static CGScope *cur_scope;
static int stack_size;

static void enter_scope(void) {
    CGScope *sc = calloc(1, sizeof(CGScope));
    sc->parent = cur_scope;
    cur_scope = sc;
}

static void leave_scope(void) {
    CGScope *old = cur_scope;
    cur_scope = old->parent;
    /* libera apenas o container, variáveis vivem até o fim da função */
    free(old);
}

static CGVar *find_var(const char *name) {
    for (CGScope *sc = cur_scope; sc; sc = sc->parent) {
        for (CGVar *v = sc->vars; v; v = v->next) {
            if (strcmp(v->name, name) == 0)
                return v;
        }
    }
    return NULL;
}

static CGVar *add_var(const char *name) {
    CGVar *v = calloc(1, sizeof(CGVar));
    stack_size += 4;                  /* sempre 4 bytes */
    v->offset = stack_size;
    v->name = name;
    v->next = cur_scope->vars;
    cur_scope->vars = v;
    return v;
}

/* ==========================================================================
 * Pré-passagem para descobrir tamanhos de pilha
 * ======================================================================= */

static void prep_function(Node *fn) {
    stack_size = 0;
    cur_scope = NULL;
    enter_scope();                    /* escopo global da função */

    /* registra parâmetros como variáveis locais */
    for (int i = 0; i < fn->arg_count; i++)
        add_var(fn->args[i]->name);

    /* percorre corpo recursivamente atrás de declarações */
    /* pequena função interna */
    void collect(Node *n) {
        if (!n) return;
        switch (n->kind) {
        case ND_BLOCK:
            enter_scope();
            for (int i = 0; i < n->stmt_count; i++)
                collect(n->stmts[i]);
            leave_scope();
            break;
        case ND_DECL:
            add_var(n->name);
            if (n->init)
                collect(n->init);
            break;
        case ND_FOR:
            enter_scope();
            if (n->init) collect(n->init);
            if (n->cond) collect(n->cond);
            if (n->inc)  collect(n->inc);
            for (int i = 0; i < n->stmt_count; i++)
                collect(n->stmts[i]);
            leave_scope();
            break;
        case ND_IF:
            collect(n->lhs);
            collect(n->rhs);
            if (n->els) collect(n->els);
            break;
        case ND_WHILE:
            collect(n->lhs);
            collect(n->rhs);
            break;
        case ND_FUNC: /* não deve ocorrer aqui */
            break;
        default:
            /* recursão padrão em lhs/rhs */
            if (n->lhs) collect(n->lhs);
            if (n->rhs) collect(n->rhs);
            for (int i = 0; i < n->arg_count; i++)
                collect(n->args[i]);
            break;
        }
    }

    for (int i = 0; i < fn->stmt_count; i++)
        collect(fn->stmts[i]);
}

/* ==========================================================================
 * Emissão de expressões e statements
 * ======================================================================= */

static int label_count = 0;

/* Emite assembly que deixa o resultado em r0 */
static void emit_expr(FILE *out, Node *n);

/* Retorna endereço da variável em r0 */
static void emit_addr(FILE *out, Node *n) {
    if (n->kind == ND_VAR) {
        CGVar *v = find_var(n->name);
        if (v) {
            fprintf(out, "    sub r0, fp, #%d\n", v->offset);
            return;
        }
        /* global */
        fprintf(out, "    ldr r0, =%s\n", n->name);
        return;
    } else if (n->kind == ND_DEREF) {
        emit_expr(out, n->lhs); /* resultado é endereço */
        return;
    }
    /* fallback */
    emit_expr(out, n);
}

static void emit_expr(FILE *out, Node *n) {
    switch (n->kind) {
    case ND_NUM:
        fprintf(out, "    mov r0, #%d\n", n->val);
        return;
    case ND_VAR: {
        CGVar *v = find_var(n->name);
        if (v) {
            fprintf(out, "    ldr r0, [fp, #%d]\n", -v->offset);
            return;
        }
        fprintf(out, "    ldr r0, =%s\n", n->name);
        fprintf(out, "    ldr r0, [r0]\n");
        return;
    }
    case ND_ADDR:
        emit_addr(out, n->lhs);
        return;
    case ND_DEREF:
        emit_expr(out, n->lhs);
        fprintf(out, "    ldr r0, [r0]\n");
        return;
    case ND_ASSIGN:
        emit_addr(out, n->lhs);            /* r0 = endereço */
        fprintf(out, "    push {r0}\n");
        emit_expr(out, n->rhs);            /* r0 = valor */
        fprintf(out, "    pop {r1}\n");
        fprintf(out, "    str r0, [r1]\n");
        return;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
        emit_expr(out, n->lhs);
        fprintf(out, "    push {r0}\n");
        emit_expr(out, n->rhs);
        fprintf(out, "    mov r1, r0\n");
        fprintf(out, "    pop {r0}\n");
        switch (n->kind) {
        case ND_ADD: fprintf(out, "    add r0, r0, r1\n"); break;
        case ND_SUB: fprintf(out, "    sub r0, r0, r1\n"); break;
        case ND_MUL: fprintf(out, "    mul r0, r0, r1\n"); break;
        case ND_DIV:
            fprintf(out, "    mov r2, r0\n");
            fprintf(out, "    mov r0, r2\n");
            fprintf(out, "    bl __aeabi_idiv\n");
            break;
        case ND_EQ:
            fprintf(out, "    cmp r0, r1\n    moveq r0, #1\n    movne r0, #0\n");
            break;
        case ND_NE:
            fprintf(out, "    cmp r0, r1\n    movne r0, #1\n    moveq r0, #0\n");
            break;
        case ND_LT:
            fprintf(out, "    cmp r0, r1\n    movlt r0, #1\n    movge r0, #0\n");
            break;
        case ND_LE:
            fprintf(out, "    cmp r0, r1\n    movle r0, #1\n    movgt r0, #0\n");
            break;
        default: break;
        }
        return;
    case ND_CALL:
        for (int i = 0; i < n->arg_count && i < 4; i++) {
            emit_expr(out, n->args[i]);
            fprintf(out, "    mov r%d, r0\n", i);
        }
        fprintf(out, "    bl %s\n", n->name);
        return;
    default:
        /* casos não tratados */
        return;
    }
}

static void emit_stmt(FILE *out, Node *n) {
    if (!n) return;
    switch (n->kind) {
    case ND_RETURN:
        if (n->lhs)
            emit_expr(out, n->lhs);
        fprintf(out, "    b .Lret\n");
        break;
    case ND_BLOCK:
        enter_scope();
        for (int i = 0; i < n->stmt_count; i++)
            emit_stmt(out, n->stmts[i]);
        leave_scope();
        break;
    case ND_DECL:
        if (n->init) {
            emit_expr(out, n->init);
            CGVar *v = find_var(n->name);
            if (v)
                fprintf(out, "    str r0, [fp, #%d]\n", -v->offset);
        }
        break;
    case ND_IF: {
        int lid = label_count++;
        emit_expr(out, n->lhs);
        fprintf(out, "    cmp r0, #0\n    beq .Lelse%d\n", lid);
        emit_stmt(out, n->rhs);
        fprintf(out, "    b .Lend%d\n", lid);
        fprintf(out, ".Lelse%d:\n", lid);
        if (n->els) emit_stmt(out, n->els);
        fprintf(out, ".Lend%d:\n", lid);
        break;
    }
    case ND_WHILE: {
        int lid = label_count++;
        fprintf(out, ".Lbegin%d:\n", lid);
        emit_expr(out, n->lhs);
        fprintf(out, "    cmp r0, #0\n    beq .Lendw%d\n", lid);
        emit_stmt(out, n->rhs);
        fprintf(out, "    b .Lbegin%d\n", lid);
        fprintf(out, ".Lendw%d:\n", lid);
        break;
    }
    case ND_FOR: {
        int lid = label_count++;
        enter_scope();
        if (n->init) emit_stmt(out, n->init);
        fprintf(out, ".Lforcond%d:\n", lid);
        if (n->cond) {
            emit_expr(out, n->cond);
            fprintf(out, "    cmp r0, #0\n    beq .Lforend%d\n", lid);
        }
        for (int i = 0; i < n->stmt_count; i++)
            emit_stmt(out, n->stmts[i]);
        if (n->inc) emit_expr(out, n->inc);
        fprintf(out, "    b .Lforcond%d\n", lid);
        fprintf(out, ".Lforend%d:\n", lid);
        leave_scope();
        break;
    }
    default:
        /* declaração ou expressão simples */
        emit_expr(out, n);
        break;
    }
}

/* ==========================================================================
 * Interface principal
 * ======================================================================= */

static void emit_function(FILE *out, Node *fn) {
    prep_function(fn);
    fprintf(out, ".global %s\n", fn->name);
    fprintf(out, "%s:\n", fn->name);
    fprintf(out, "    push {fp, lr}\n");
    fprintf(out, "    mov fp, sp\n");
    if (stack_size)
        fprintf(out, "    sub sp, sp, #%d\n", stack_size);

    /* salva parâmetros em slots locais */
    int param_regs = fn->arg_count < 4 ? fn->arg_count : 4;
    for (int i = 0; i < param_regs; i++) {
        CGVar *v = find_var(fn->args[i]->name);
        if (v)
            fprintf(out, "    str r%d, [fp, #%d]\n", i, -v->offset);
    }

    for (int i = 0; i < fn->stmt_count; i++)
        emit_stmt(out, fn->stmts[i]);

    fprintf(out, ".Lret:\n");
    if (stack_size)
        fprintf(out, "    add sp, fp, #0\n");
    fprintf(out, "    pop {fp, pc}\n");
    fprintf(out, "\n");
}

void codegen_to_file(Node *root, const char *out_path) {
    FILE *f = fopen(out_path, "w");
    if (!f) {
        perror(out_path);
        return;
    }

    fprintf(f, ".text\n");

    /* primeiro declara globais (na seção .data) */
    int has_data = 0;
    for (int i = 0; i < root->stmt_count; i++) {
        Node *n = root->stmts[i];
        if (n->kind == ND_DECL) {
            if (!has_data) {
                fprintf(f, ".data\n");
                has_data = 1;
            }
            if (n->init && n->init->kind == ND_NUM)
                fprintf(f, "%s: .word %d\n", n->name, n->init->val);
            else
                fprintf(f, "%s: .word 0\n", n->name);
        }
    }

    fprintf(f, ".text\n");

    for (int i = 0; i < root->stmt_count; i++) {
        Node *n = root->stmts[i];
        if (n->kind == ND_FUNC)
            emit_function(f, n);
    }

    fclose(f);
}




