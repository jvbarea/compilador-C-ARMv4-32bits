#ifndef LEXER_H
#define LEXER_H

#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif

// Lê todo o arquivo em memória; quem chamar deve liberar o retorno
char *read_file(const char *path);

// Tokeniza um buffer em memória; NÃO libera src
Token *tokenize(const char *src);

// Wrapper legado: read_file + tokenize + free(src)
Token *tokenize_file(const char *path);

// Imprime tokens até TK_EOF
void print_tokens(Token *tokens);

// Libera o vetor de tokens e cada lexema alocado
void free_tokens(Token *tokens);

#ifdef __cplusplus
}
#endif

#endif // LEXER_H