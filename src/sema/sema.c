/* src/sema/sema.c
 * Implementação da análise semântica mínima (Sema)
 */

#include "sema.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "token.h"

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
    // aqui supomos que variáveis e literais são todos inteiros
    // se quiser suportar outros tipos, deve-se usar node->type
    if (node->lhs->kind == ND_NUM || node->lhs->kind == ND_VAR) {
        /* ok */
    } else {
        report_error_ctx(ctx,"Operador aplicado a não-inteiro (lhs)", node);
        ctx->error_reported = true;
    }
    if (node->rhs->kind == ND_NUM || node->rhs->kind == ND_VAR) {
        /* ok */
    } else {
        report_error_ctx(ctx,"Operador aplicado a não-inteiro (rhs)", node);
        ctx->error_reported = true;
    }
}

SemaErrorCode sema_analyze(SemaContext *ctx, Node *root) {
    if (!root) return SEMA_OK;

    switch (root->kind) {

    case ND_BLOCK:
        sema_enter_scope(ctx);
        for (int i = 0; i < root->stmt_count; i++) {
            sema_analyze(ctx, root->stmts[i]);
        }
        sema_leave_scope(ctx);
        break;

    case ND_DECL:
        // declara variável ou função-local (parser não separa global/local aqui)
        if (sema_declare(ctx, root->name, ND_DECL, NULL) != SEMA_OK) {
            report_error_ctx(ctx,"Redeclaração de identificador", root);
            ctx->error_reported = true;
        }
        if (root->init) {
            sema_analyze(ctx, root->init);
        }
        break;

    case ND_FUNC:
        // registro do nome da função no escopo global
        if (sema_declare(ctx, root->name, ND_FUNC, NULL) != SEMA_OK) {
            report_error_ctx(ctx,"Redeclaração de função", root);
            ctx->error_reported = true;
        }
        // escopo para parâmetros e corpo
        sema_enter_scope(ctx);
        for (int i = 0; i < root->arg_count; i++) {
            Node *param = root->args[i];
            if (sema_declare(ctx, param->name, ND_DECL, NULL) != SEMA_OK) {
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
        if (!sema_resolve(ctx, root->name)) {
            report_error_ctx(ctx,"Identificador não declarado", root);
            ctx->error_reported = true;
        }
        break;

    case ND_NUM:
        // literal numérico: nada a checar
        break;

    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
        sema_analyze(ctx, root->lhs);
        sema_analyze(ctx, root->rhs);
        check_binary_int(ctx, root);
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

    case ND_CALL:
        // analisa cada argumento
        for (int i = 0; i < root->arg_count; i++) {
            sema_analyze(ctx, root->args[i]);
        }
        // poderia checar número de args vs. declaração de função
        break;

    default:
        // outros kinds (ND_LOGAND, ND_LOGOR, comparações, post-inc/dec...)
        // devem ser implementados conforme necessidade
        break;
    }

    return ctx->error_reported ? SEMA_TYPE_MISMATCH : SEMA_OK;
}
