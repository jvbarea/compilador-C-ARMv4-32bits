#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer/lexer.h"

int main(int argc, char **argv) {
    if (argc != 3 || strcmp(argv[1], "-tokens") != 0) {
        fprintf(stderr, "Uso: %s -tokens <arquivo.c>\n", argv[0]);
        return 1;
    }

    // 1) lê todo o arquivo em memória
    char *src = read_file(argv[2]);
    if (!src) {
        fprintf(stderr, "Erro ao ler arquivo %s\n", argv[2]);
        return 1;
    }

    // 2) tokeniza o buffer sem liberar src
    Token *toks = tokenize(src);
    if (!toks) {
        fprintf(stderr, "Erro no lexer para %s\n", argv[2]);
        free(src);
        return 1;
    }

    // 3) imprime tokens até TK_EOF
    print_tokens(toks);

    // 4) libera tokens e lexemas
    free_tokens(toks);

    // 5) libera buffer de código-fonte
    free(src);

    return 0;
}
