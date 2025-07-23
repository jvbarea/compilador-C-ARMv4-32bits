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

// conta '*' consecutivos; avança o cursor, devolve quantos
static int count_stars(void) {
    int n = 0;
    while (consume(TK_SYM_STAR))
        n++;
    return n;
}

// AST node constructors
// AST node constructors (recebem Token *tok para localização)
static Node *new_node(Token *tok, NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind  = kind;
    node->token = tok;
    return node;
}

static Node *new_node_binary(Token *tok, NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(tok, kind);
    node->lhs  = lhs;
    node->rhs  = rhs;
    return node;
}

static Node *new_node_unary(Token *tok, NodeKind kind, Node *expr) {
    Node *node = new_node(tok, kind);
    node->lhs  = expr;
    return node;
}

static Node *new_node_num(Token *tok) {
    Node *node = new_node(tok, ND_NUM);
    node->val  = tok->ival;
    return node;
}

static Node *new_node_var(Token *tok) {
    Node *node = new_node(tok, ND_VAR);
    node->name = copy_str(tok->lexeme);
    return node;
}

static Node *new_node_call(Token *tok, const char *name, Node **args, int argc) {
    Node *node = new_node(tok, ND_CALL);
    node->name      = copy_str(name);
    node->args      = args;
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
    // ( expr )
    if (consume(TK_SYM_LPAREN)) {
        Node *node = parse_expression();
        expect(TK_SYM_RPAREN);
        return node;
    }

    // literal numérico
    if (cur->kind == TK_NUM) {
        Token *tok = cur;
        // long   v   = tok->ival;
        next();
        Node *node = new_node_num(tok);  // sua factory antiga
        node->token = tok;             // só aqui guardamos a localização
        return node;
    }

    // identificador ou chamada de função
    if (cur->kind == TK_IDENT) {
        Token *tok  = cur;
        char  *name = copy_str(tok->lexeme);
        next();

        // chamada de função?
        if (consume(TK_SYM_LPAREN)) {
            Node **args = NULL;
            int    argc = 0;
            if (!consume(TK_SYM_RPAREN)) {
                do {
                    Node *arg = parse_expression();
                    args = realloc(args, sizeof(Node*) * (argc + 1));
                    args[argc++] = arg;
                } while (consume(TK_SYM_COMMA));
                expect(TK_SYM_RPAREN);
            }
            Node *call = new_node_call(tok ,name, args, argc);
            call->token = tok;
            return call;
        }

        // simples uso de variável
        Node *var = new_node_var(tok);
        var->token = tok;
        return var;
    }

    error_at(cur, "expected primary expression");
    return NULL;
}


static Node *parse_postfix(void) {
    Node *n = parse_primary();
    for (;;) {
        // pós-incremento
        if (peek(0)->kind == TK_INC) {
            Token *tok = next();  // consome '++' e guarda o token
            n = new_node_unary(tok, ND_POSTINC, n);
            continue;
        }
        // pós-decremento
        if (peek(0)->kind == TK_DEC) {
            Token *tok = next();  // consome '--' e guarda o token
            n = new_node_unary(tok, ND_POSTDEC, n);
            continue;
        }
        break;
    }
    return n;
}


// Unary: +, -, !, then primary
static Node *parse_unary(void) {

    /* &expr : operador de endereço */
    if (peek(0)->kind == TK_SYM_AMP) {
        Token *tok = next();              // consome '&'
        Node  *sub = parse_unary();       // avalia o operando recursivamente
        Node  *n   = new_node_unary(tok, ND_ADDR, sub);
        /* type será ajustado no Sema; se quiser já adiantar: */
        /* n->type = pointer_to(sub->type); */
        return n;
    }

    /* *expr : operador de dereferência */
    if (peek(0)->kind == TK_SYM_STAR) {
        Token *tok = next();              // consome '*'
        Node  *sub = parse_unary();
        Node  *n   = new_node_unary(tok, ND_DEREF, sub);
        /* Deixe n->type = NULL; o Sema checará se sub->type é ponteiro
           e então fará n->type = sub->type->base */
        return n;
    }

    /* +expr : apenas ignora o '+' */
    if (peek(0)->kind == TK_SYM_PLUS) {
        next();                           // consome '+'
        return parse_unary();
    }

    /* -expr : transforma em 0 - expr */
    if (peek(0)->kind == TK_SYM_MINUS) {
        Token *tok = next();              // consome '-'
        Node *zero = new_node_num(tok);   // literal 0 com mesmo token
        zero->val  = 0;
        return new_node_binary(tok, ND_SUB, zero, parse_unary());
    }

    /* caso geral → postfix / primary */
    return parse_postfix();
}

// Multiplicative: *, /
static Node *parse_multiplicative(void) {
    Node *node = parse_unary();
    for (;;) {
        // *
        if (peek(0)->kind == TK_SYM_STAR) {
            Token *tok = next();  // consome '*'
            node = new_node_binary(tok, ND_MUL, node, parse_unary());
            continue;
        }
        // /
        if (peek(0)->kind == TK_SYM_SLASH) {
            Token *tok = next();  // consome '/'
            node = new_node_binary(tok, ND_DIV, node, parse_unary());
            continue;
        }
        break;
    }
    return node;
}


// Additive: +, -
static Node *parse_additive(void) {
    Node *node = parse_multiplicative();
    for (;;) {
        // +
        if (peek(0)->kind == TK_SYM_PLUS) {
            Token *tok = next();  // consome '+' e captura o token
            node = new_node_binary(tok, ND_ADD, node, parse_multiplicative());
            continue;
        }
        // -
        if (peek(0)->kind == TK_SYM_MINUS) {
            Token *tok = next();  // consome '-' e captura o token
            node = new_node_binary(tok, ND_SUB, node, parse_multiplicative());
            continue;
        }
        break;
    }
    return node;
}


// Relational: <, <=, >, >=
static Node *parse_relational(void) {
    Node *node = parse_additive();
    for (;;) {
        // <
        if (peek(0)->kind == TK_SYM_LT) {
            Token *tok = next();  // consome '<'
            node = new_node_binary(tok, ND_LT, node, parse_additive());
            continue;
        }
        // <=
        if (peek(0)->kind == TK_LE) {
            Token *tok = next();  // consome '<='
            node = new_node_binary(tok, ND_LE, node, parse_additive());
            continue;
        }
        // >
        if (peek(0)->kind == TK_SYM_GT) {
            Token *tok = next();  // consome '>'
            // transforma 'a > b' em 'b < a', reutilizando tok para localização
            node = new_node_binary(tok, ND_LT, parse_additive(), node);
            continue;
        }
        // >=
        if (peek(0)->kind == TK_GE) {
            Token *tok = next();  // consome '>='
            // transforma 'a >= b' em 'b <= a'
            node = new_node_binary(tok, ND_LE, parse_additive(), node);
            continue;
        }
        break;
    }
    return node;
}


// Equality: ==, !=
static Node *parse_equality(void) {
    Node *node = parse_relational();
    for (;;) {
        // ==
        if (peek(0)->kind == TK_EQ) {
            Token *tok = next();  // consome '=='
            node = new_node_binary(tok, ND_EQ, node, parse_relational());
            continue;
        }
        // !=
        if (peek(0)->kind == TK_NEQ) {
            Token *tok = next();  // consome '!='
            node = new_node_binary(tok, ND_NE, node, parse_relational());
            continue;
        }
        break;
    }
    return node;
}


// Logical AND: &&
static Node *parse_logical_and(void) {
    Node *node = parse_equality();
    for (;;) {
        // &&
        if (peek(0)->kind == TK_AND) {
            Token *tok = next();  // consome '&&'
            node = new_node_binary(tok, ND_LOGAND, node, parse_equality());
            continue;
        }
        break;
    }
    return node;
}

// Logical OR: ||
static Node *parse_logical_or(void) {
    Node *node = parse_logical_and();
    for (;;) {
        // ||
        if (peek(0)->kind == TK_OR) {
            Token *tok = next();  // consome '||'
            node = new_node_binary(tok, ND_LOGOR, node, parse_logical_and());
            continue;
        }
        break;
    }
    return node;
}

// Assignment: = (right-associative)
static Node *parse_assignment(void) {
    Node *node = parse_logical_or();
    // =
    if (peek(0)->kind == TK_SYM_ASSIGN) {
        Token *tok = next();  // consome '='
        node = new_node_binary(tok, ND_ASSIGN, node, parse_assignment());
    }
    return node;
}

// Expression entry
static Node *parse_expression(void) {
    return parse_assignment();
}
static Node *parse_statement(void) {
    // 1) bloco aninhado
    if (peek(0)->kind == TK_SYM_LBRACE)
        return parse_compound();

    // 2) return-stmt
    if (peek(0)->kind == TK_KW_RETURN) {
        Token *tok = next();            // consome 'return'
        Node *n = new_node(tok, ND_RETURN);
        n->lhs = parse_expression();
        expect(TK_SYM_SEMI);
        return n;
    }

    // 3) if-else
    if (peek(0)->kind == TK_KW_IF) {
        Token *tok = next();            // consome 'if'
        expect(TK_SYM_LPAREN);
        Node *cond = parse_expression();
        expect(TK_SYM_RPAREN);
        Node *then_branch = parse_statement();
        Node *else_branch = NULL;
        if (peek(0)->kind == TK_KW_ELSE) {
            next();                      // consome 'else'
            else_branch = parse_statement();
        }
        Node *n = new_node(tok, ND_IF);
        n->lhs = cond;
        n->rhs = then_branch;
        n->els = else_branch;
        return n;
    }

    // 4) while
    if (peek(0)->kind == TK_KW_WHILE) {
        Token *tok = next();            // consome 'while'
        expect(TK_SYM_LPAREN);
        Node *cond = parse_expression();
        expect(TK_SYM_RPAREN);
        Node *body = parse_statement();
        Node *n = new_node(tok, ND_WHILE);
        n->lhs = cond;
        n->rhs = body;
        return n;
    }

    // 5) for
    if (peek(0)->kind == TK_KW_FOR) {
        Token *tok = next();            // consome 'for'
        expect(TK_SYM_LPAREN);
        // init
        Node *init = NULL;
        if (peek(0)->kind != TK_SYM_SEMI) {
            if (peek(0)->kind == TK_KW_INT) {
                // Token *idt = next();    // consome 'int'
                next();    // consome 'int'
                int stars = count_stars(); 
                Token *id  = expect(TK_IDENT);
                init = new_node(id, ND_DECL);
                init->name = copy_str(id->lexeme);
                if (peek(0)->kind == TK_SYM_ASSIGN) {
                    next();
                    init->init = parse_expression();
                }
            } else {
                init = parse_expression();
            }
        }
        expect(TK_SYM_SEMI);

        // cond
        Node *cond = NULL;
        if (peek(0)->kind != TK_SYM_SEMI) {
            cond = parse_expression();
        }
        expect(TK_SYM_SEMI);

        // inc
        Node *inc = NULL;
        if (peek(0)->kind != TK_SYM_RPAREN) {
            inc = parse_expression();
        }
        expect(TK_SYM_RPAREN);

        // body
        Node *body = parse_statement();
        Node *n = new_node(tok, ND_FOR);
        n->init = init;
        n->cond = cond;
        n->inc  = inc;
        n->rhs  = body;
        return n;
    }

    // 6) declaração local simples: int x; int y = expr;
    if (peek(0)->kind == TK_KW_INT) {
        Token *tok_int = next();        // consome 'int'
        Node **decls = NULL;
        int    cnt   = 0;
        do {
            int   stars = count_stars();
            Token *id   = expect(TK_IDENT);
            Node *n = new_node(id, ND_DECL);
            n->name = copy_str(id->lexeme);
            if (peek(0)->kind == TK_SYM_ASSIGN) {
                next();                  // consome '='
                n->init = parse_expression();
            }
            decls = realloc(decls, sizeof(Node*) * (cnt + 1));
            decls[cnt++] = n;
        } while (peek(0)->kind == TK_SYM_COMMA && (next(), 1));
        expect(TK_SYM_SEMI);

        if (cnt == 1) {
            Node *only = decls[0];
            free(decls);
            return only;
        }
        Node *blk = new_node(tok_int, ND_BLOCK);
        blk->stmts      = decls;
        blk->stmt_count = cnt;
        return blk;
    }

    // 7) expression-stmt
    {
        Node *expr = parse_expression();
        expect(TK_SYM_SEMI);
        return expr;
    }
}

static Node *parse_compound(void) {
    // consome '{' e captura token para localização do bloco
    Token *tok = expect(TK_SYM_LBRACE);

    // cria nó de bloco com token do '{'
    Node *blk = new_node(tok, ND_BLOCK);
    blk->stmts      = NULL;
    blk->stmt_count = 0;

    // até encontrar '}'...
    while (peek(0)->kind != TK_SYM_RBRACE) {
        Node *st = parse_statement();

        // achata blocos que vieram de declarações múltiplas
        if (st->kind == ND_BLOCK &&
            st->stmt_count > 0 &&
            st->stmts[0]->kind == ND_DECL) {
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
            // bloco normal ou outro statement
            blk->stmts = realloc(
                blk->stmts,
                sizeof(Node*) * (blk->stmt_count + 1)
            );
            blk->stmts[blk->stmt_count++] = st;
        }
    }

    // consome '}' final
    expect(TK_SYM_RBRACE);
    return blk;
}

static Node *parse_global_decl(void) {
    // consome 'int' (token não usado para localização da declaração em si)
    expect(TK_KW_INT);

    // nome da variável global
    Token *id = expect(TK_IDENT);

    // cria nó de declaração usando o token do identificador
    Node *n = new_node(id, ND_DECL);
    n->name = copy_str(id->lexeme);

    // inicializador opcional: = expr
    if (peek(0)->kind == TK_SYM_ASSIGN) {
        next();  // consome '='
        n->init = parse_expression();
    }

    // ponto-e-vírgula final
    expect(TK_SYM_SEMI);
    return n;
}

// Função: parse_function_decl
// Reconhece: int <name>( params ) { body }
static Node *parse_function_decl(void) {
    // 1) palavra-chave 'int'
    // Token *tok_int = expect(TK_KW_INT);
    expect(TK_KW_INT);

    // 2) nome da função
    Token *fn = expect(TK_IDENT);

    // 3) parêntese de abertura
    expect(TK_SYM_LPAREN);

    // 4) parâmetros formais
    Node **params = NULL;
    int    pcount = 0;
    if (peek(0)->kind != TK_SYM_RPAREN) {
        do {
            // cada parâmetro começa com 'int'
            expect(TK_KW_INT);
            int stars = count_stars();
            // nome do parâmetro
            Token *pt = expect(TK_IDENT);
            // cria nó ND_VAR para o parâmetro, usando o token do identificador
            Node *p = new_node(pt, ND_VAR);
            p->name = copy_str(pt->lexeme);

            params = realloc(params, sizeof(Node*) * (pcount + 1));
            params[pcount++] = p;
        } while (consume(TK_SYM_COMMA));
    }
    // fecha a lista de parâmetros
    expect(TK_SYM_RPAREN);

    // 5) corpo da função (bloco composto)
    Node *body = parse_compound();  // já consome '{' … '}' internamente

    // 6) monta o nó ND_FUNC, usando o token do identificador da função
    Node *fnode = new_node(fn, ND_FUNC);
    fnode->name       = copy_str(fn->lexeme);
    fnode->args       = params;
    fnode->arg_count  = pcount;
    // reaproveita statements do bloco interno
    fnode->stmts      = body->stmts;
    fnode->stmt_count = body->stmt_count;
    free(body);

    return fnode;
}

// Program: sequence of globals and functions
Node *parse_program(Token *tok_stream) {
    // inicializa stream de tokens
    cur = tok_stream;

    // cria nó bloco raiz usando o primeiro token para localização
    Token *root_tok = peek(0);
    Node *root = new_node(root_tok, ND_BLOCK);
    root->stmts      = NULL;
    root->stmt_count = 0;

    // enquanto não chegarmos ao EOF
    while (peek(0)->kind != TK_EOF) {
        Node *node;
        // lookahead a 2 tokens: identificador seguido de '(' indica função
        if (peek(0)->kind == TK_KW_INT &&
            peek(1)->kind == TK_IDENT &&
            peek(2)->kind == TK_SYM_LPAREN) {
            node = parse_function_decl();
        } else {
            node = parse_global_decl();
        }
        // adiciona ao bloco raiz
        root->stmts = realloc(
            root->stmts,
            sizeof(Node*) * (root->stmt_count + 1)
        );
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
