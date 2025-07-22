#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer/lexer.h"   // já declara read_file, tokenize, print_tokens, free_tokens
#include "parser/parser.h" // declara parse_program, free_node, Node, etc.

// Imprime AST em formato prefixado
static void print_ast(Node *n, int indent)
{
    if (!n)
        return;
    for (int i = 0; i < indent; i++)
        putchar(' ');
    switch (n->kind)
    {
    case ND_NUM:
        printf("(NUM %d)\n", n->val);
        break;
    case ND_VAR:
        printf("(VAR %s)\n", n->name);
        break;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_ASSIGN:
    {
        const char *op =
            n->kind == ND_ADD ? "ADD" : n->kind == ND_SUB ? "SUB"
                                    : n->kind == ND_MUL   ? "MUL"
                                    : n->kind == ND_DIV   ? "DIV"
                                    : n->kind == ND_EQ    ? "EQ"
                                    : n->kind == ND_NE    ? "NE"
                                    : n->kind == ND_LT    ? "LT"
                                    : n->kind == ND_LE    ? "LE"
                                                          : "ASSIGN";
        printf("(%s\n", op);
        print_ast(n->lhs, indent + 2);
        print_ast(n->rhs, indent + 2);
        break;
    }
    case ND_CALL:
        printf("(CALL %s\n", n->name);
        for (int i = 0; i < n->arg_count; i++)
            print_ast(n->args[i], indent + 2);
        break;
    case ND_DECL:
        printf("(DECL %s\n", n->name);
        if (n->init)
            print_ast(n->init, indent + 2);
        break;
    case ND_RETURN:
        printf("(RETURN\n");
        if (n->lhs)
            print_ast(n->lhs, indent + 2);
        break;
    case ND_IF:
        printf("(IF\n");
        // condição
        print_ast(n->lhs, indent + 2);
        // then
        print_ast(n->rhs, indent + 2);
        // else, se existir
        if (n->els)
        {
            for (int i = 0; i < indent + 2; i++)
                putchar(' ');
            printf("(ELSE\n");
            print_ast(n->els, indent + 4);
            for (int i = 0; i < indent + 2; i++)
                putchar(' ');
            printf(")\n");
        }
        break;
    case ND_WHILE:
        printf("(WHILE\n");
        // imprime cond
        print_ast(n->lhs, indent + 2);
        // imprime corpo
        print_ast(n->rhs, indent + 2);
        break;
    case ND_FOR:
        printf("(FOR\n");
        if (n->init)
            print_ast(n->init, indent + 2);
        if (n->cond)
            print_ast(n->cond, indent + 2);
        if (n->inc)
            print_ast(n->inc, indent + 2);
        if (n->rhs)
            print_ast(n->rhs, indent + 2);
        break;
    case ND_POSTINC:
        printf("(POSTINC\n");
        print_ast(n->lhs, indent + 2);
        break;
    case ND_POSTDEC:
        printf("(POSTDEC\n");
        print_ast(n->lhs, indent + 2);
        break;
    case ND_BLOCK:
        printf("(BLOCK\n");
        for (int i = 0; i < n->stmt_count; i++)
            print_ast(n->stmts[i], indent + 2);
        break;
    case ND_FUNC:
        printf("(FUNC %s\n", n->name);
        for (int i = 0; i < n->arg_count; i++)
            print_ast(n->args[i], indent + 2);
        for (int i = 0; i < n->stmt_count; i++)
            print_ast(n->stmts[i], indent + 2);
        break;
    default:
        printf("(UNKNOWN %d)\n", n->kind);
    }

    if (n->kind != ND_NUM && n->kind != ND_VAR)
    {
        for (int i = 0; i < indent; i++)
            putchar(' ');
        printf(")\n");
    }
}

int main(int argc, char **argv)
{
    if (argc != 3 ||
        (strcmp(argv[1], "-tokens") != 0 && strcmp(argv[1], "-ast") != 0))
    {
        fprintf(stderr, "Uso: %s [-tokens|-ast] <arquivo.c>\n", argv[0]);
        return 1;
    }

    // 1) Leitura do arquivo (usa read_file de lexer.c)
    char *src = read_file(argv[2]);
    if (!src)
    {
        perror("read_file");
        return 1;
    }

    // 2) Tokenização
    Token *toks = tokenize(src);
    if (!toks)
    {
        fprintf(stderr, "Erro no lexer\n");
        free(src);
        return 1;
    }

    // 3) Escolha de modo
    if (strcmp(argv[1], "-tokens") == 0)
    {
        print_tokens(toks);
    }
    else
    {
        Node *root = parse_program(toks);
        print_ast(root, 0);
        free_node(root);
    }

    // 4) Cleanup
    free_tokens(toks);
    free(src);
    return 0;
}
