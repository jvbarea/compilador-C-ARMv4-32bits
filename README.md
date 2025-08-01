# Compilador C para ARMv4-32bits

Este repositório contém um pequeno compilador escrito em C capaz de traduzir um subconjunto da linguagem C para assembly ARMv4 de 32 bits. O projeto serve de base para estudos de construção de compiladores e está dividido em módulos bem definidos.

## Etapas da compilação

1. **Análise léxica** (`src/lexer`)
   - Percorre o arquivo fonte completo em memória e converte caracteres em *tokens*.
   - Remove comentários e espaços em branco e classifica palavras‑chave, identificadores, literais e símbolos.
2. **Análise sintática** (`src/parser`)
   - Consome a lista de tokens e constrói uma **Árvore de Sintaxe Abstrata (AST)** por meio de parser preditivo recursivo.
   - Reconhece declarações de funções, variáveis globais e comandos como `if`, `while` e `for`.
3. **Análise semântica** (`src/sema`)
   - Percorre a AST, mantém tabelas de símbolos para variáveis e funções e valida tipos e escopos.
   - Emite mensagens de erro detalhadas caso encontre uso de identificadores não declarados ou tipos incompatíveis.
4. **Geração de código** (`src/code_generator`)
   - Converte a AST anotada em assembly ARM.  Cada função recebe prólogo e epílogo fixos e são utilizados no máximo quatro registradores, com *spilling* na pilha quando necessário.

A etapa de geração ainda está em evolução, mas já produz `.s` para programas simples.

## Estrutura do repositório

```
mycc/
├── Makefile               # regras para compilar o compilador e executar testes
├── include/               # cabeçalhos públicos
│   ├── token.h            # definição de Token e enum TokenKind
│   └── stubs.h            # protótipos auxiliares (malloc, printf…)
├── src/                   # código-fonte do compilador
│   ├── lexer/             # analisador léxico
│   ├── parser/            # parser e estruturas de AST
│   ├── sema/              # analisador semântico
│   ├── code_generator/    # gerador de assembly
│   └── main.c             # programa principal que orquestra as fases
└── tests/                 # casos de teste de cada etapa
```

## Uso

Compile o executável `mycc` com `make`:

```
$ make
```

Em seguida é possível executar cada fase de forma independente:

```
./mycc -tokens arquivo.c   # imprime a lista de tokens
./mycc -ast arquivo.c      # imprime a AST em formato prefixado
./mycc -sema arquivo.c     # executa a análise semântica (padrão)
./mycc -S arquivo.c        # gera assembly ARM no arquivo .s correspondente
```

Os scripts de teste em `tests/` automatizam a compilação de exemplos e a verificação de saída.

## Objetivos da Fase 1

O projeto está na **Fase 1**, cujo foco é ter um compilador mínimo que suporte:

- Tipo `int` e literais inteiros ou de caractere.
- Variáveis globais e locais, funções com até quatro argumentos.
- Comandos `if/else`, `for`, `while` e `return`.
- Geração de assembly com prólogo/epílogo fixo e uso de até quatro registradores.

Concluída essa fase, a próxima etapa é ampliar a checagem de tipos (incluindo ponteiros) e evoluir o gerador de código.

## Referências

As implementações e explicações das fases de compilação foram baseadas em notas de aula e materiais sobre construção de compiladores. O arquivo `projeto.md` contém uma visão resumida das etapas planejadas.

