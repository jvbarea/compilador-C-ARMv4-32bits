#ifndef TOKEN_H
#define TOKEN_H
#include <stddef.h>

// Token kinds
typedef enum {
    TK_EOF = 0,
    TK_KW_INT,
    TK_KW_RETURN,
    TK_KW_CHAR,
    TK_KW_IF,
    TK_KW_ELSE,
    TK_KW_WHILE,
    TK_KW_FOR,
    TK_IDENT,
    TK_NUM,
    TK_SYM_PLUS,    // '+'
    TK_SYM_MINUS,   // '-'
    TK_SYM_STAR,    // '*'
    TK_SYM_SLASH,   // '/'
    TK_AND,         // '&&'
    TK_OR,          // '||'
    TK_SYM_SEMI,    // ';'
    TK_SYM_AMP,     // '&'
    TK_SYM_COMMA,   // ','
    TK_SYM_LPAREN,  // '('
    TK_SYM_RPAREN,  // ')'
    TK_SYM_LBRACE,  // '{'
    TK_SYM_RBRACE,  // '}'
    TK_SYM_LT,      // '<'
    TK_SYM_GT,      // '>'
    TK_SYM_ASSIGN,  // '='
    TK_INC,         // '++'
    TK_DEC,         // '--'
    TK_EQ,          // '=='
    TK_NEQ,         // '!='
    TK_LE,          // '<='
    TK_GE           // '>='
} TokenKind;

// Token structure
typedef struct Token {
    TokenKind kind;
    const char *lexeme;
    size_t len;
    int line, col;
    int ival;        // valor literal para TK_NUM
} Token;

#endif