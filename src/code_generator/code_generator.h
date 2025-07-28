#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "../parser/parser.h"

/*
 * Gera arquivo de assembly (.s) para a AST completa.
 * Por simplicidade assumimos que todos os tipos são `int` ou ponteiros e que o
 * programa cabe em registradores ARM básicos.
 */
void codegen_to_file(Node *root, const char *out_path);

#endif /* CODE_GENERATOR_H */
