#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

// Forward declarations
static Token *tokenize_buffer(const char *buf);

// Lê arquivo inteiro em buffer e tokeniza
Token *tokenize_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) {
        perror("malloc");
        exit(1);
    }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    Token *tokens = tokenize_buffer(buf);
    free(buf);
    return tokens;
}

// Tokeniza string de código
Token *tokenize(const char *src) {
    return tokenize_buffer(src);
}

// Imprime tokens
void print_tokens(Token *tokens) {
    for (Token *t = tokens; t->kind != TK_EOF; ++t) {
        printf("%2d:%2d %-12d '", t->line, t->col, t->kind);
        fwrite(t->lexeme, 1, t->len, stdout);
        printf("'\n");
    }
    printf("%2d:%2d %-12d '<EOF>'\n", tokens->line, tokens->col, TK_EOF);
}

// Implementação simplificada de tokenize_buffer
static Token *tokenize_buffer(const char *p) {
    size_t cap = 128, len = 0;
    Token *tokens = malloc(cap * sizeof(Token));
    int line = 1, col = 1;
    #define EMIT(kind, start, l) do {                                         \
        if (len+1 >= cap) { cap *= 2;                                         \
            tokens = realloc(tokens, cap * sizeof(Token));                    \
            if (!tokens) { perror("realloc"); exit(1); }                      \
        }                                                                     \
        /* copia o lexema para memória própria */                             \
        char *lex = malloc((l) + 1);                                          \
        if (!lex) { perror("malloc"); exit(1); }                              \
        memcpy(lex, start, (l));                                              \
        lex[(l)] = '\0';                                                      \
        tokens[len++] = (Token){kind, lex, l, line, col,                      \
            (kind==TK_NUM ? strtol(start, NULL, 0) : 0)};                     \
        col += l;                                                             \
    } while (0)

    while (*p) {
        if (*p == '\n') { p++; line++; col = 1; continue; }
        if (isspace(*p)) { p++; col++; continue; }
        // Comentários //
        if (p[0]=='/' && p[1]=='/') { while (*p && *p!='\n') p++; continue; }
        // Comentários /* */
        if (p[0]=='/' && p[1]=='*') { p+=2; while (p[0]&&!(p[0]=='*'&&p[1]=='/')) p++; p+=2; continue; }
        // Identificador / palavra-chave
        if (isalpha(*p) || *p=='_') {
            const char *start = p;
            while (isalnum(*p)||*p=='_') p++;
            size_t l = p - start;
            TokenKind k = TK_IDENT;
            // palavras-chave:
            if (l==3 && !memcmp(start,"int",3)) k = TK_KW_INT;
            else if (l==6 && !memcmp(start,"return",6)) k=TK_KW_RETURN;
            else if (l==4 && !memcmp(start,"char",4)) k=TK_KW_CHAR;
            else if (l==2 && !memcmp(start,"if",2)) k=TK_KW_IF;
            else if (l==4 && !memcmp(start,"else",4)) k=TK_KW_ELSE;
            else if (l==5 && !memcmp(start,"while",5)) k=TK_KW_WHILE;
            else if (l==3 && !memcmp(start,"for",3)) k=TK_KW_FOR;
            EMIT(k, start, l);
            continue;
        }
        // Número literal
        if (isdigit(*p)) {
            const char *start = p;
            if (p[0]=='0' && (p[1]=='x'||p[1]=='X')) {
                p+=2; while (isxdigit(*p)) p++;
            } else {
                while (isdigit(*p)) p++;
            }
            EMIT(TK_NUM, start, p-start);
            continue;
        }
        // Símbolos de 2 chars
        if (p[0]=='='&&p[1]=='=') { EMIT(TK_EQ,p,2); p+=2; continue; }
        if (p[0]=='!'&&p[1]=='=') { EMIT(TK_NEQ,p,2); p+=2; continue; }
        if (p[0]=='<'&&p[1]=='=') { EMIT(TK_LE,p,2); p+=2; continue; }
        if (p[0]=='>'&&p[1]=='=') { EMIT(TK_GE,p,2); p+=2; continue; }
        // Símbolos de 1 char
        TokenKind sk = TK_EOF;
        switch (*p) {
#define CASE(c,kind) case c: sk=kind; break
            CASE('+',TK_SYM_PLUS);
            CASE('-',TK_SYM_MINUS);
            CASE('*',TK_SYM_STAR);
            CASE('/',TK_SYM_SLASH);
            CASE(';',TK_SYM_SEMI);
            CASE(',',TK_SYM_COMMA);
            CASE('(',TK_SYM_LPAREN);
            CASE(')',TK_SYM_RPAREN);
            CASE('{',TK_SYM_LBRACE);
            CASE('}',TK_SYM_RBRACE);
            CASE('<',TK_SYM_LT);
            CASE('>',TK_SYM_GT);
            CASE('=',TK_SYM_ASSIGN);
        default:
            fprintf(stderr,"%d:%d: caractere inválido '%c'\n", line, col, *p);
            exit(1);
        }
        EMIT(sk, p, 1);
        p++;
    }
    EMIT(TK_EOF, p, 0);
    return tokens;
}