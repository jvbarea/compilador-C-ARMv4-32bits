#ifndef TYPE_H
#define TYPE_H

typedef enum { TY_INT, TY_PTR, TY_FUNC, /* ... */ } TypeKind;

typedef struct Type {
    TypeKind kind;
    struct Type *base;     // para ponteiros, arrays etc.
    struct Type **params;  // para funções
    int param_count;
} Type;

#endif // TYPE_H
