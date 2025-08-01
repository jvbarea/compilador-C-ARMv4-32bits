#include "type.h"
#include <stdlib.h>

/* singletons para tipos base */
// static Type int_s  = { TY_INT };
// Type *ty_int = &int_s;


Type *ty_int;
Type *ty_void;

void init_types(void) {
    ty_int  = malloc(sizeof(Type));
    ty_int->kind = TY_INT;
    ty_int->base = NULL;
    ty_int->params = NULL;
    ty_int->param_count = 0;

    ty_void = malloc(sizeof(Type));
    ty_void->kind = TY_VOID;
    ty_void->base = NULL;
    ty_void->params = NULL;
    ty_void->param_count = 0;
}
/* helpers */
Type *pointer_to(Type *base) {
    Type *t = malloc(sizeof(Type));
    t->kind = TY_PTR;
    t->base = base;
    t->params = NULL;
    t->param_count = 0;
    return t;
}

Type *func_type(Type *ret, Type **params, int n) {
    Type *t = malloc(sizeof(Type));
    t->kind = TY_FUNC;
    t->base = ret;
    t->params = params;
    t->param_count = n;
    return t;
}
