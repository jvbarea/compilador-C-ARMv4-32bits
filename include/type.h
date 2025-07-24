#ifndef TYPE_H
#define TYPE_H

typedef enum { TY_INT, TY_PTR, TY_FUNC, TY_VOID /* ... */ } TypeKind;

typedef struct Type {
    TypeKind kind;
    struct Type *base;     // para ponteiros, arrays etc.
    struct Type **params;  // para funções
    int param_count;
} Type;

extern Type *ty_int;
extern Type *ty_void;

void init_types(void);

Type *pointer_to(Type *base);
Type *func_type(Type *ret, Type **params, int n);

#endif // TYPE_H
