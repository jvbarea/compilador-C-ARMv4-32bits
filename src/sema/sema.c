/* src/sema/sema.c
 * Implementação da análise semântica mínima (Sema)
 */

#include "sema.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "token.h"
#include "type.h"

// Cria um novo escopo, linkando-o ao pai
static SemaScope *new_scope(SemaScope *parent) {
    SemaScope *sc = calloc(1, sizeof(SemaScope));
    sc->parent = parent;
    return sc;
}

void sema_init(SemaContext *ctx) {
    ctx->current_scope = new_scope(NULL);
    ctx->error_reported = false;
}

void sema_enter_scope(SemaContext *ctx) {
    ctx->current_scope = new_scope(ctx->current_scope);
}

void sema_leave_scope(SemaContext *ctx) {
    SemaScope *old = ctx->current_scope;
    ctx->current_scope = old->parent;

    // libera símbolos do escopo antigo
    SemaSymbol *sym = old->symbols;
    while (sym) {
        SemaSymbol *next = sym->next;
        free(sym);
        sym = next;
    }
    free(old);
}

SemaErrorCode sema_declare(SemaContext *ctx, const char *name, NodeKind kind, Type *type) {
    // verifica redeclaração no escopo atual
    for (SemaSymbol *it = ctx->current_scope->symbols; it; it = it->next) {
        if (strcmp(it->name, name) == 0) {
            return SEMA_REDECLARED_IDENT;
        }
    }
    // insere novo símbolo
    SemaSymbol *sym = calloc(1, sizeof(SemaSymbol));
    sym->name        = name;
    sym->kind        = kind;
    sym->type        = type;
    sym->stack_offset = 0;  // será ajustado no codegen
    sym->next        = ctx->current_scope->symbols;
    ctx->current_scope->symbols = sym;
    return SEMA_OK;
}

SemaSymbol *sema_resolve(SemaContext *ctx, const char *name) {
    for (SemaScope *sc = ctx->current_scope; sc; sc = sc->parent) {
        for (SemaSymbol *sym = sc->symbols; sym; sym = sym->next) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
        }
    }
    return NULL;
}

// Reporta erro semântico com localização
static void report_error_ctx(SemaContext *ctx,
                             const char *msg,
                             const Node *node) {
    if (node && node->token) {
        fprintf(stderr,
                "%d:%d: erro semântico: %s em '%.*s'\n",
                node->token->line,
                node->token->col,
                msg,
                (int)node->token->len,
                node->token->lexeme);
    } else {
        fprintf(stderr, "erro semântico: %s\n", msg);
    }
    ctx->error_reported = true;
}


// Função auxiliar para checar binários inteiros
static void check_binary_int(SemaContext *ctx, Node *node) {
    Node *l = node->lhs;
    Node *r = node->rhs;

    /* caso 1: int  + int  → int  */
    if (l->type->kind == TY_INT && r->type->kind == TY_INT) {
        node->type = ty_int;
        return;
    }

    /* caso 2: ptr + int / int + ptr → ptr (aritmética de ponteiro simples) */
    if (l->type->kind == TY_PTR && r->type->kind == TY_INT) {
        node->type = l->type;
        return;
    }
    if (r->type->kind == TY_PTR && l->type->kind == TY_INT) {
        node->type = r->type;
        return;
    }

    report_error_ctx(ctx, "tipos incompatíveis para operador aritmético", node);
}

SemaErrorCode sema_analyze(SemaContext *ctx, Node *root) {
    if (!root) return SEMA_OK;
    if (root && root->token) {
        fprintf(stderr,
                "[sema] visiting kind=%d at %d:%d\n",
                root->kind,
                root->token->line,
                root->token->col);
    }
    switch (root->kind) {

    case ND_BLOCK:
        int need_scope = ctx->current_scope->parent != NULL;
        if(need_scope) sema_enter_scope(ctx);
        for (int i = 0; i < root->stmt_count; i++) {
            sema_analyze(ctx, root->stmts[i]);
        }
        if (need_scope) sema_leave_scope(ctx);
        break;
    case ND_DEREF:
        sema_analyze(ctx, root->lhs);
        if (root->lhs->type->kind != TY_PTR)
            report_error_ctx(ctx, "operador * exige ponteiro", root);
        else
            root->type = root->lhs->type->base;
        break;

    case ND_DECL:
        // declara variável ou função-local (parser não separa global/local aqui)
        if (sema_declare(ctx, root->name, ND_DECL, root->type) != SEMA_OK) {
            report_error_ctx(ctx,"Redeclaração de identificador", root);
            ctx->error_reported = true;
        }
        if (root->init) {
            sema_analyze(ctx, root->init);
            // opcional: aqui poderíamos checar root->init->type vs root->type
        }
        break;

    case ND_FUNC:
        // registro do nome da função no escopo global
        if (sema_declare(ctx, root->name, ND_FUNC, root->type) != SEMA_OK) {
            report_error_ctx(ctx,"Redeclaração de função", root);
            ctx->error_reported = true;
        }
        // escopo para parâmetros e corpo
        sema_enter_scope(ctx);
        for (int i = 0; i < root->arg_count; i++) {
            Node *param = root->args[i];
            if (sema_declare(ctx, param->name, ND_DECL, param->type) != SEMA_OK) {
                report_error_ctx(ctx,"Redeclaração de parâmetro", param);
                ctx->error_reported = true;
            }
        }
        for (int i = 0; i < root->stmt_count; i++) {
            sema_analyze(ctx, root->stmts[i]);
        }
        sema_leave_scope(ctx);
        break;

    case ND_VAR:
        // uso de variável: deve já ter sido declarada
        SemaSymbol *sym = sema_resolve(ctx, root->name);
        if (!sym) {
             report_error_ctx(ctx,"Identificador não declarado", root);
             ctx->error_reported = true;
        } else {
            root->type = sym->type;     /* → propaga o tipo para a AST */
        }
        break;

    case ND_NUM:
        // literal numérico: já tipado como inteiro
        root->type = ty_int;
        break;
        break;

    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
        sema_analyze(ctx, root->lhs);
        sema_analyze(ctx, root->rhs);
        check_binary_int(ctx, root);
        break;
    // ─────── Atribuição ─────────────────────────────────────────────────
    case ND_ASSIGN:
        // visita lhs e rhs
        sema_analyze(ctx, root->lhs);
        sema_analyze(ctx, root->rhs);
        // opcional: exigir tipos compatíveis (int ← int, ptr ← ptr)
        if (!root->lhs->type || !root->rhs->type ||
            root->lhs->type->kind != root->rhs->type->kind) {
            report_error_ctx(ctx, "tipos incompatíveis em atribuição", root);
        }
        root->type = root->lhs->type;
        break;

    // ─────── Relacionais (<, <=, >, >=) ────────────────────────────────
    // case ND_LT: case ND_LE: case ND_GT: case ND_GE:
    case ND_LT: case ND_LE:
        sema_analyze(ctx, root->lhs);
        sema_analyze(ctx, root->rhs);
        // só faça comparações entre inteiros
        if (root->lhs->type->kind != TY_INT || root->rhs->type->kind != TY_INT) {
            report_error_ctx(ctx, "comparação exige inteiros", root);
        }
        root->type = ty_int;  // comparações produzem int (0 ou 1)
        break;

    // ─────── Igualdade (==, !=) ─────────────────────────────────────────
    case ND_EQ: case ND_NE:
        sema_analyze(ctx, root->lhs);
        sema_analyze(ctx, root->rhs);
        // permita comparar ptr ou int, mas ambos lados devem ser do mesmo kind
        if (!root->lhs->type || !root->rhs->type ||
            root->lhs->type->kind != root->rhs->type->kind) {
            report_error_ctx(ctx, "tipos incompatíveis para ==/!=", root);
        }
        root->type = ty_int;
        break;
    case ND_RETURN:
        if (root->lhs) {
            sema_analyze(ctx, root->lhs);
        }
        break;

    case ND_IF:
        sema_analyze(ctx, root->lhs);        // condição
        sema_analyze(ctx, root->rhs);        // then-branch
        if (root->els) sema_analyze(ctx, root->els);
        break;

    case ND_WHILE:
        sema_analyze(ctx, root->lhs);        // condição
        sema_analyze(ctx, root->rhs);        // corpo
        break;

    case ND_FOR:
        if (root->init) sema_analyze(ctx, root->init);
        if (root->cond) sema_analyze(ctx, root->cond);
        if (root->inc ) sema_analyze(ctx, root->inc);
        sema_enter_scope(ctx);
        for (int i = 0; i < root->stmt_count; i++) {
            sema_analyze(ctx, root->stmts[i]);
        }
        sema_leave_scope(ctx);
        break;

    case ND_ADDR:
        sema_analyze(ctx, root->lhs);
        root->type = pointer_to(root->lhs->type);
        break;
    case ND_POSTINC:
    case ND_POSTDEC:
        // primeiro tipa o operando
        sema_analyze(ctx, root->lhs);
        // só permita inteiros (ou ponteiros, se quiser)
        if (root->lhs->type->kind != TY_INT) {
            report_error_ctx(ctx,
                "operador ++/-- exige inteiro", root);
        }
        // o tipo da expressão é o mesmo do operando
        root->type = root->lhs->type;
        break;
    case ND_LOGAND:
    case ND_LOGOR:
        // tipa ambos lados
        sema_analyze(ctx, root->lhs);
        sema_analyze(ctx, root->rhs);
        // exigimos inteiros como condição
        if (root->lhs->type->kind != TY_INT ||
            root->rhs->type->kind != TY_INT) {
            report_error_ctx(ctx,
                "operador lógico exige inteiros", root);
        }
        // resultado de && e || também é int (0 ou 1)
        root->type = ty_int;
        break;

    case ND_CALL: {
        /* a função é guardada em root->lhs (um ND_VAR)  */
        sema_analyze(ctx, root->lhs);
        if (!root->lhs->type) {
            report_error_ctx(ctx, "chamada de função sem declaração prévia", root);
            break;
        }
        /* visita e tipa cada argumento */
        for (int i = 0; i < root->arg_count; i++)
            sema_analyze(ctx, root->args[i]);

        Type *fnty = root->lhs->type;
        if (!fnty || fnty->kind != TY_FUNC) {
            report_error_ctx(ctx,"tentativa de chamar não-função", root);
            break;
        }
        if (root->arg_count != fnty->param_count) {
            report_error_ctx(ctx,"nº de argumentos diferente do declarado", root);
            break;
        }
        /* (opcional) comparar cada arg[i]->type com fnty->params[i] */
        root->type = fnty->base;   /* tipo de retorno */
        break;
    }


    default:
        // outros kinds (ND_LOGAND, ND_LOGOR, comparações, post-inc/dec...)
        // devem ser implementados conforme necessidade
        break;
    }

    return ctx->error_reported ? SEMA_TYPE_MISMATCH : SEMA_OK;
}
