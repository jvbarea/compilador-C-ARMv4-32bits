#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"

// Grammar (BNF):
// program        ::= (function_decl | global_decl)*
// function_decl ::= type ident '(' (param_list)? ')' compound_stmt
// global_decl   ::= type ident ('=' expression)? ';'
// type          ::= 'int'
// param_list    ::= type ident (',' type ident)*
// compound_stmt ::= '{' statement* '}'
// statement     ::= compound_stmt
//                 | 'return' expression ';'
//                 | 'if' '(' expression ')' statement ('else' statement)?
//                 | 'while' '(' expression ')' statement
//                 | 'for' '(' (declaration | expr_stmt)? ';' expr? ';' expr? ')' statement
//                 | expr_stmt
// expr_stmt     ::= expression? ';'
// expression    ::= assignment
// assignment    ::= logical_or ( '=' assignment )?
// logical_or    ::= logical_and ( '||' logical_and)*
// logical_and   ::= equality ( '&&' equality)*
// equality      ::= relational ( ('==' | '!=') relational)*
// relational    ::= additive ( ('<' | '>' | '<=' | '>=') additive)*
// additive      ::= multiplicative ( ('+' | '-') multiplicative)*
// multiplicative::= unary ( ('*' | '/' | '%') unary)*
// unary         ::= ('+' | '-' | '!' | '++' | '--') unary | primary
// primary       ::= NUMBER | IDENT | '(' expression ')' | function_call
// function_call ::= ident '(' (argument_list)? ')'
// argument_list ::= expression (',' expression)*

// AST Node kinds
typedef enum {
    ND_ADD, ND_SUB, ND_MUL, ND_DIV,ND_LOGAND, ND_LOGOR,
    ND_EQ, ND_NE, ND_LT, ND_LE,
    ND_ASSIGN, ND_VAR, ND_NUM,
    ND_RETURN, ND_IF, ND_WHILE, ND_FOR,
    ND_BLOCK, ND_POSTINC,
    ND_POSTDEC, ND_CALL, ND_FUNC, ND_DECL,
} NodeKind;

// AST Node structure
typedef struct Node {
    NodeKind kind;
    struct Node *lhs;         // left-hand side or condition
    struct Node *rhs;         // right-hand side or then-branch
    struct Node *els;         // else-branch
    int val;                  // used for ND_NUM
    char *name;               // identifiers, function names
    struct Node **args;       // argument list for calls
    int arg_count;
    struct Node **stmts;      // block statements
    int stmt_count;
    struct Node *init, *cond, *inc; // for ND_FOR
} Node;

// Parse the entire program; returns root node or NULL on error
typedef struct Function Function;
Node *parse_program(Token *tok);

// Free the AST recursively
void free_node(Node *node);

#endif // PARSER_H
