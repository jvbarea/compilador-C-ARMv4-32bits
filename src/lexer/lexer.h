#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gera tokens a partir de string de código
Token *tokenize(const char *src);
// Gera tokens a partir de arquivo .c
Token *tokenize_file(const char *path);
// Imprime tokens até TK_EOF
void print_tokens(Token *tokens);

#ifdef __cplusplus
}
#endif
#endif // LEXER_H