// src/lexer/lexer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

// forward declaration
static Token *tokenize_buffer(const char *buf);

// Lê arquivo inteiro em buffer (terminado em '\0')
char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    if (!buf) { perror("malloc"); exit(1); }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

// Tokeniza um buffer em memória; não libera buf
Token *tokenize(const char *buf) {
    return tokenize_buffer(buf);
}

// Wrapper legado: lê + tokeniza + libera buf
Token *tokenize_file(const char *path) {
    char *buf = read_file(path);
    Token *toks = tokenize(buf);
    free(buf);
    return toks;
}

// Imprime tokens até TK_EOF
void print_tokens(Token *tokens) {
    for (Token *t = tokens; t->kind != TK_EOF; ++t) {
        printf("%2d:%2d %-12d '", t->line, t->col, t->kind);
        fwrite(t->lexeme, 1, t->len, stdout);
        printf("'\n");
    }
    printf("%2d:%2d %-12d '<EOF>'\n", tokens->line, tokens->col, TK_EOF);
}

// Libera cada lexema e o vetor de tokens
void free_tokens(Token *tokens) {
    for (Token *t = tokens; t->kind != TK_EOF; ++t) {
        free((char *)t->lexeme);
    }
    free(tokens);
}

// Implementação de tokenize_buffer (igual à anterior)
static Token *tokenize_buffer(const char *p) {
    size_t cap = 128, len = 0;
    Token *tokens = malloc(cap * sizeof(Token));
    int line = 1, col = 1;
    #define EMIT(kind, start, l) do {                                         \
        if (len+1 >= cap) { cap *= 2;                                         \
            tokens = realloc(tokens, cap * sizeof(Token));                    \
            if (!tokens) { perror("realloc"); exit(1); }                    \
        }                                                                     \
        char *lex = malloc((l) + 1);                                          \
        if (!lex) { perror("malloc"); exit(1); }                            \
        memcpy(lex, start, (l));                                              \
        lex[(l)] = '\0';                                                      \
        tokens[len++] = (Token){kind, lex, l, line, col,                      \
            (kind==TK_NUM ? strtol(start, NULL, 0) : 0)};                     \
        col += l;                                                             \
    } while (0)

    while (*p) {
        if (*p == '\n') { p++; line++; col = 1; continue; }
        if (isspace((unsigned char)*p)) { p++; col++; continue; }
        if (p[0]=='/' && p[1]=='/') { while (*p && *p!='\n') p++; continue; }
        if (p[0]=='/' && p[1]=='*') { p+=2; while (p[0] && !(p[0]=='*'&&p[1]=='/')) p++; p+=2; continue; }
        if (isalpha((unsigned char)*p) || *p=='_') {
            const char *start = p;
            while (isalnum((unsigned char)*p)||*p=='_') p++;
            size_t l = p - start;
            TokenKind k = TK_IDENT;
            if (l==3 && !memcmp(start,"int",3)) k = TK_KW_INT;
            else if (l==6 && !memcmp(start,"return",6)) k=TK_KW_RETURN;
            else if (l==2 && !memcmp(start,"if",2)) k=TK_KW_IF;
            else if (l==4 && !memcmp(start,"else",4)) k=TK_KW_ELSE;
            else if (l==5 && !memcmp(start,"while",5)) k=TK_KW_WHILE;
            else if (l==3 && !memcmp(start,"for",3)) k=TK_KW_FOR;
            EMIT(k, start, l);
            continue;
        }
        if (isdigit((unsigned char)*p)) {
            const char *start = p;
            if (p[0]=='0' && (p[1]=='x'||p[1]=='X')) { p+=2; while (isxdigit((unsigned char)*p)) p++; }
            else { while (isdigit((unsigned char)*p)) p++; }
            EMIT(TK_NUM, start, p-start);
            continue;
        }
        if (p[0]=='='&&p[1]=='=') { EMIT(TK_EQ,p,2); p+=2; continue; }
        if (p[0]=='!'&&p[1]=='=') { EMIT(TK_NEQ,p,2); p+=2; continue; }
        if (p[0]=='<'&&p[1]=='=') { EMIT(TK_LE,p,2); p+=2; continue; }
        if (p[0]=='>'&&p[1]=='=') { EMIT(TK_GE,p,2); p+=2; continue; }
        if (p[0]=='&'&&p[1]=='&') { EMIT(TK_AND,p,2); p+=2; continue; }
        if (p[0]=='|'&&p[1]=='|') { EMIT(TK_OR,p,2); p+=2; continue; }
        TokenKind sk;
        switch (*p) {
            case '+': sk = TK_SYM_PLUS;   break;
            case '-': sk = TK_SYM_MINUS;  break;
            case '*': sk = TK_SYM_STAR;   break;
            case '/': sk = TK_SYM_SLASH;  break;
            case ';': sk = TK_SYM_SEMI;   break;
            case ',': sk = TK_SYM_COMMA;  break;
            case '(': sk = TK_SYM_LPAREN; break;
            case ')': sk = TK_SYM_RPAREN; break;
            case '{': sk = TK_SYM_LBRACE; break;
            case '}': sk = TK_SYM_RBRACE; break;
            case '<': sk = TK_SYM_LT;     break;
            case '>': sk = TK_SYM_GT;     break;
            case '=': sk = TK_SYM_ASSIGN; break;
            default:
                fprintf(stderr, "%d:%d: caractere inválido '%c'\n", line, col, *p);
                exit(1);
        }
        EMIT(sk, p, 1);
        p++;
    }
    EMIT(TK_EOF, p, 0);
    return tokens;
}