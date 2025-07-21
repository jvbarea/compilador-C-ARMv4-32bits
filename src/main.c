// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "lexer.h"

// static char *read_file(const char *path){
//     FILE *f = fopen(path,"rb");
//     if(!f){ perror(path); exit(1);}
//     fseek(f,0,SEEK_END); long sz = ftell(f); rewind(f);
//     char *buf = malloc(sz+1); if(!buf){perror("malloc"); exit(1);}
//     fread(buf,1,sz,f); buf[sz]='\0'; fclose(f);
//     return buf;
// }

// int main(int argc,char **argv){
//     if(argc!=3 || strcmp(argv[1],"-tokens")){
//         fprintf(stderr,"uso: %s -tokens <arquivo.c>\n",argv[0]);
//         return 1;
//     }
//     char *src = read_file(argv[2]);
//     Token *toks = tokenize(src);
//     for(size_t i=0; toks[i].kind!=TK_EOF; ++i){
//         Token *t=&toks[i];
//         printf("%4d:%-2d  %-8s  %.*s", t->line, t->col,
//                tok_kind_to_str(t->kind), (int)t->len, t->lexeme);
//         if(t->kind==TK_NUM) printf("  (%ld)",t->ival);
//         putchar('\n');
//     }
//     free(toks);
//     free(src);
//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer/lexer.h"

int main(int argc, char **argv)
{
    if (argc != 3 || strcmp(argv[1], "-tokens")){
        fprintf(stderr, "Uso: %s -tokens <arquivo.c>\n", argv[0]);
        return 1;
    }
    Token *toks = tokenize_file(argv[2]);
    print_tokens(toks);
    for (Token *t = toks; t->kind != TK_EOF; ++t){
        free((char *)t->lexeme);
    }
    free(toks);
    return 0;
}