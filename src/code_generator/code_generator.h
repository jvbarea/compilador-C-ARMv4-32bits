#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H
#include "../parser/parser.h"
void codegen_to_file(Node *root, const char *out_path);
#endif
