#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "../lexer/lexer.h"

// Token stream pointer
static Token *cur;

// Error reporting
static void error_at(Token *tok, const char *msg) {
    fprintf(stderr, "%d:%d: %s\n", tok->line, tok->col, msg);
    exit(1);
}

static char *copy_str(const char *s) {
    size_t l = strlen(s);
    char *r = malloc(l + 1);
    if (!r) { perror("malloc"); exit(1); }
    memcpy(r, s, l + 1);
    return r;
}

// Stream navigation helpers
static Token *peek(int n) {
    return &cur[n];
}
static Token *next(void) {
    return cur++;
}
static int consume(TokenKind kind) {
    if (cur->kind == kind) {
        cur++;
        return 1;
    }
    return 0;
}
static Token *expect(TokenKind kind) {
    if (cur->kind != kind) {
        error_at(cur, "unexpected token");
    }
    return next();
}

// AST node constructors
static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}
static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}
static Node *new_node_unary(NodeKind kind, Node *expr) {
    Node *n = new_node(kind);
    n->lhs = expr;
    return n;
}

static Node *new_node_num(long val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}
static Node *new_node_var(const char *name) {
    Node *node = new_node(ND_VAR);
    node->name = copy_str(name);
    return node;
}
static Node *new_node_call(const char *name, Node **args, int argc) {
    Node *node = new_node(ND_CALL);
    node->name = copy_str(name);
    node->args = args;
    node->arg_count = argc;
    return node;
}

// Forward declarations for recursive functions
static Node *parse_expression(void);
static Node *parse_assignment(void);
static Node *parse_logical_or(void);
static Node *parse_logical_and(void);
static Node *parse_equality(void);
static Node *parse_relational(void);
static Node *parse_additive(void);
static Node *parse_multiplicative(void);
static Node *parse_unary(void);
static Node *parse_primary(void);
static Node *parse_postfix(void);
static Node *parse_statement(void);
static Node *parse_compound(void);
static Node *parse_global_decl(void);
static Node *parse_function_decl(void);

// Primary expressions: numbers, identifiers, calls, parentheses
static Node *parse_primary(void) {
    if (consume(TK_SYM_LPAREN)) {
        Node *node = parse_expression();
        expect(TK_SYM_RPAREN);
        return node;
    }
    if (cur->kind == TK_NUM) {
        Node *node = new_node_num(cur->ival);
        next();
        return node;
    }
    if (cur->kind == TK_IDENT) {
        const char *name = cur->lexeme;
        next();
        // function call?
        if (consume(TK_SYM_LPAREN)) {
            Node **args = NULL;
            int argc = 0;
            if (!consume(TK_SYM_RPAREN)) {
                do {
                    Node *arg = parse_expression();
                    args = realloc(args, sizeof(Node*) * (argc + 1));
                    args[argc++] = arg;
                } while (consume(TK_SYM_COMMA));
                expect(TK_SYM_RPAREN);
            }
            return new_node_call(name, args, argc);
        }
        return new_node_var(name);
    }
    error_at(cur, "expected primary expression");
    return NULL;
}

static Node *parse_postfix(void) {
    Node *n = parse_primary();
    for (;;) {
        if (consume(TK_INC))
            n = new_node_unary(ND_POSTINC, n);
        else if (consume(TK_DEC))
            n = new_node_unary(ND_POSTDEC, n);
        else
            break;
    }
    return n;
}

// Unary: +, -, !, then primary
static Node *parse_unary(void) {
    if (consume(TK_SYM_PLUS))
        return parse_unary();
    if (consume(TK_SYM_MINUS))
        return new_node_binary(ND_SUB, new_node_num(0), parse_unary());
    return parse_postfix();
}

// Multiplicative: *, /
static Node *parse_multiplicative(void) {
    Node *node = parse_unary();
    for (;;) {
        if (consume(TK_SYM_STAR))
            node = new_node_binary(ND_MUL, node, parse_unary());
        else if (consume(TK_SYM_SLASH))
            node = new_node_binary(ND_DIV, node, parse_unary());
        else
            break;
    }
    return node;
}

// Additive: +, -
static Node *parse_additive(void) {
    Node *node = parse_multiplicative();
    for (;;) {
        if (consume(TK_SYM_PLUS))
            node = new_node_binary(ND_ADD, node, parse_multiplicative());
        else if (consume(TK_SYM_MINUS))
            node = new_node_binary(ND_SUB, node, parse_multiplicative());
        else
            break;
    }
    return node;
}

// Relational: <, <=, >, >=
static Node *parse_relational(void) {
    Node *node = parse_additive();
    for (;;) {
        if      (consume(TK_SYM_LT))  // '<'
            node = new_node_binary(ND_LT, node, parse_additive());
        else if (consume(TK_LE))      // '<='
            node = new_node_binary(ND_LE, node, parse_additive());
        else if (consume(TK_SYM_GT))  // '>'
            // a > b  é equivalente a  b < a
            node = new_node_binary(ND_LT, parse_additive(), node);
        else if (consume(TK_GE))      // '>='
            // a >= b  é equivalente a  b <= a
            node = new_node_binary(ND_LE, parse_additive(), node);
        else
            break;
    }
    return node;
}

// Equality: ==, !=
static Node *parse_equality(void) {
    Node *node = parse_relational();
    for (;;) {
        if (consume(TK_EQ))
            node = new_node_binary(ND_EQ, node, parse_relational());
        else if (consume(TK_NEQ))
            node = new_node_binary(ND_NE, node, parse_relational());
        else
            break;
    }
    return node;
}

// Logical AND: &&
static Node *parse_logical_and(void) {
    Node *node = parse_equality();
    while (consume(TK_AND))
        node = new_node_binary(ND_LOGAND, node, parse_equality());
    return node;
}

// Logical OR: ||
static Node *parse_logical_or(void) {
    Node *node = parse_logical_and();
    while (consume(TK_OR))
        node = new_node_binary(ND_LOGOR, node, parse_logical_and());
    return node;
}

// Assignment: = (right-associative)
static Node *parse_assignment(void) {
    Node *node = parse_logical_or();
    if (consume(TK_SYM_ASSIGN))
        node = new_node_binary(ND_ASSIGN, node, parse_assignment());
    return node;
}

// Expression entry
static Node *parse_expression(void) {
    return parse_assignment();
}

// Statement and other parsing functions (to be implemented)
static Node *parse_statement(void) {
    // 1) bloco aninhado
    if (peek(0)->kind == TK_SYM_LBRACE)
        return parse_compound();

    // 2) return-stmt
    if (consume(TK_KW_RETURN)) {
        Node *n = new_node(ND_RETURN);
        n->lhs = parse_expression();
        expect(TK_SYM_SEMI);
        return n;
    }

    // 3) if-else
    if (consume(TK_KW_IF)) {
        expect(TK_SYM_LPAREN);
        Node *cond = parse_expression();
        expect(TK_SYM_RPAREN);
        Node *then_branch = parse_statement();
        Node *else_branch = NULL;
        if (consume(TK_KW_ELSE))
            else_branch = parse_statement();
        Node *n = new_node(ND_IF);
        n->lhs = cond;
        n->rhs = then_branch;
        n->els = else_branch;
        return n;
    }

    // 4) while
    if (consume(TK_KW_WHILE)) {
        expect(TK_SYM_LPAREN);
        Node *cond = parse_expression();
        expect(TK_SYM_RPAREN);
        Node *body = parse_statement();
        Node *n = new_node(ND_WHILE);
        n->lhs = cond;
        n->rhs = body;
        return n;
    }

    // 5) for
    if (consume(TK_KW_FOR)) {
        expect(TK_SYM_LPAREN);
        // init
        Node *init = NULL;
        if (!consume(TK_SYM_SEMI)) {
            if (consume(TK_KW_INT)) {
                Token *id = expect(TK_IDENT);
                init = new_node(ND_DECL);
                init->name = copy_str(id->lexeme);
                if (consume(TK_SYM_ASSIGN))
                    init->init = parse_expression();
            } else {
                init = parse_expression();
            }
            expect(TK_SYM_SEMI);
        }
        // cond
        Node *cond = NULL;
        if (!consume(TK_SYM_SEMI)) {
            cond = parse_expression();
            expect(TK_SYM_SEMI);
        }
        // inc
        Node *inc = NULL;
        if (!consume(TK_SYM_RPAREN)) {
            inc = parse_expression();
            expect(TK_SYM_RPAREN);
        }
        // body
        Node *body = parse_statement();
        Node *n = new_node(ND_FOR);
        n->init = init;
        n->cond = cond;
        n->inc  = inc;
        n->rhs  = body;
        return n;
    }

    // 6) declaração local simples: int x; int y = expr;
    // local declaration: int a=…[, b=…]*;
    if (consume(TK_KW_INT)) {
        Node **decls = NULL;
        int    cnt   = 0;
        do {
            Token *id = expect(TK_IDENT);
            Node  *n  = new_node(ND_DECL);
            n->name   = copy_str(id->lexeme);
            if (consume(TK_SYM_ASSIGN))
                n->init = parse_expression();
            decls = realloc(decls, sizeof(Node*) * (cnt+1));
            decls[cnt++] = n;
        } while (consume(TK_SYM_COMMA));
        expect(TK_SYM_SEMI);
        if (cnt == 1) {
            Node *only = decls[0];
            free(decls);
            return only;
        }
        // se veio >1, embrulha num bloco
        Node *blk = new_node(ND_BLOCK);
        blk->stmts      = decls;
        blk->stmt_count = cnt;
        return blk;
    }

    // 7) expressionstmt
    {
        Node *expr = parse_expression();
        expect(TK_SYM_SEMI);
        return expr;
    }
}

static Node *parse_compound(void) {
    expect(TK_SYM_LBRACE);
    Node *blk = new_node(ND_BLOCK);
    blk->stmts      = NULL;
    blk->stmt_count = 0;

    while (!consume(TK_SYM_RBRACE)) {
        Node *st = parse_statement();

        // Se for um bloco criado por “int a=… , b=…;” (só ND_DECL dentro)
        if (st->kind == ND_BLOCK &&
            st->stmt_count > 0 &&
            st->stmts[0]->kind == ND_DECL) {
            // achata: puxa cada declaração para o bloco pai
            for (int i = 0; i < st->stmt_count; i++) {
                blk->stmts = realloc(
                    blk->stmts,
                    sizeof(Node*) * (blk->stmt_count + 1)
                );
                blk->stmts[blk->stmt_count++] = st->stmts[i];
            }
            free(st->stmts);
            free(st);
        } else {
            // bloco normal ou qualquer outro statement
            blk->stmts = realloc(
                blk->stmts,
                sizeof(Node*) * (blk->stmt_count + 1)
            );
            blk->stmts[blk->stmt_count++] = st;
        }
    }

    return blk;
}

static Node *parse_global_decl(void) {
    expect(TK_KW_INT);
    Token *id = expect(TK_IDENT);
    Node *n = new_node(ND_DECL);
    n->name = copy_str(id->lexeme);
    if (consume(TK_SYM_ASSIGN)) {
        n->init = parse_expression();
    }
    expect(TK_SYM_SEMI);
    return n;
}
static Node *parse_function_decl(void) {
    // assinatura: int <name>( params ) { body }
    expect(TK_KW_INT);
    Token *fn = expect(TK_IDENT);
    expect(TK_SYM_LPAREN);

    // parâmetros
    Node **params = NULL;
    int    pcount = 0;
    if (!consume(TK_SYM_RPAREN)) {
        do {
            expect(TK_KW_INT);
            Token *pt = expect(TK_IDENT);
            Node *p = new_node(ND_VAR);
            p->name = copy_str(pt->lexeme);
            params = realloc(params, sizeof(Node*)*(pcount+1));
            params[pcount++] = p;
        } while (consume(TK_SYM_COMMA));
        expect(TK_SYM_RPAREN);
    }

    // corpo da função como bloco composto
    Node *body = parse_compound();  // consome '{' ... '}', retorna ND_BLOCK

    // monta o nó de função
    Node *fnode = new_node(ND_FUNC);
    fnode->name       = copy_str(fn->lexeme);
    fnode->args       = params;
    fnode->arg_count  = pcount;
    // reaproveita diretamente os statements do bloco
    fnode->stmts      = body->stmts;
    fnode->stmt_count = body->stmt_count;
    free(body);
    return fnode;
}


// Program: sequence of globals and functions
Node *parse_program(Token *tok) {
    cur = tok;
    Node *root = new_node(ND_BLOCK);
    root->stmts = NULL;
    root->stmt_count = 0;

    while (!consume(TK_EOF)) {
        Node *node;
        // Decide based on lookahead if it's a function or global
        if (peek(2)->kind == TK_SYM_LPAREN)
            node = parse_function_decl();
        else
            node = parse_global_decl();

        root->stmts = realloc(root->stmts, sizeof(Node*) * (root->stmt_count + 1));
        root->stmts[root->stmt_count++] = node;
    }

    return root;
}

// Free AST nodes recursively
void free_node(Node *node) {
    if (!node) return;
    // Free children
    free_node(node->lhs);
    free_node(node->rhs);
    for (int i = 0; i < node->arg_count; i++)
        free_node(node->args[i]);
    free(node->args);
    for (int i = 0; i < node->stmt_count; i++)
        free_node(node->stmts[i]);
    free(node->stmts);
    free((char*)node->name);
    free(node);
}
