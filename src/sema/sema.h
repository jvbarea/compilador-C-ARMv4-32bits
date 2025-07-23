/* src/sema/sema.h    
 * Interface para análise semântica (Sema) do compilador didático.
 * Usa a definição de AST gerada pelo parser e tipos básicos em type.h (em include/)
 */

#ifndef SEMA_H
#define SEMA_H

#include "../parser/parser.h"   // Definição de Node, NodeKind e FunctionDecl
#include "type.h"       // Definição de Type em include/
#include <stdbool.h>

// Tipo de erro semântico
typedef enum {
    SEMA_OK = 0,
    SEMA_UNDECLARED_IDENT,    // Identificador não declarado
    SEMA_REDECLARED_IDENT,    // Identificador já declarado no mesmo escopo
    SEMA_TYPE_MISMATCH,       // Operação entre tipos incompatíveis
    SEMA_TOO_MANY_ARGS,       // Chamada de função com mais argumentos que parâmetros
    SEMA_ARG_TYPE_MISMATCH,   // Tipo de argumento diferente do parâmetro
    // ... outros códigos de erro
} SemaErrorCode;

// Informação sobre um símbolo (variável ou função)
typedef struct SemaSymbol {
    const char *name;      // Nome do símbolo
    NodeKind kind;         // ND_VAR ou ND_FUNC (definidos em parser/parser.h)
    Type *type;            // Tipo (int, ponteiro, função, etc.) definido em include/type.h
    int stack_offset;      // Deslocamento no frame (para variáveis locais)
    struct SemaSymbol *next;
} SemaSymbol;

// Escopo: lista encadeada de símbolos + link para escopo pai
typedef struct SemaScope {
    SemaSymbol *symbols;
    struct SemaScope *parent;
} SemaScope;

// Contexto global da análise semântica
typedef struct {
    SemaScope *current_scope;
    bool error_reported;
} SemaContext;

// Inicializa o contexto (chamar no início da compilação)
void sema_init(SemaContext *ctx);

// Entra em novo escopo aninhado
void sema_enter_scope(SemaContext *ctx);

// Sai do escopo atual
void sema_leave_scope(SemaContext *ctx);

// Declara um símbolo no escopo atual
SemaErrorCode sema_declare(SemaContext *ctx, const char *name, NodeKind kind, Type *type);

// Resolve um nome no contexto (procura em escopos pai)
SemaSymbol *sema_resolve(SemaContext *ctx, const char *name);

// Roda a análise semântica completa na AST raiz
SemaErrorCode sema_analyze(SemaContext *ctx, Node *root);

#endif // SEMA_H
