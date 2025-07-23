#include "type.h"
#include <stdlib.h>

/* singletons para tipos base */
static Type int_s  = { TY_INT };
Type *ty_int = &int_s;

/* helpers */
Type *pointer_to(Type *base) {
    Type *t = calloc(1, sizeof *t);
    t->kind = TY_PTR;
    t->base = base;
    return t;
}

Type *func_type(Type *ret, Type **params, int n) {
    Type *t = calloc(1, sizeof *t);
    t->kind       = TY_FUNC;
    t->base       = ret;      // retorno
    t->params     = params;
    t->param_count = n;
    return t;
}
