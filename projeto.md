# Projeto: Compilador C para ARMv4 32 bits

Este repositório contém um compilador didático em C capaz de traduzir um subconjunto da linguagem C para assembly ARMv4 de 32 bits. O objetivo é evoluir o projeto em fases bem definidas até que possamos autocompilá-lo.

## Visão geral do fluxo de compilação

1. **Lexer**: varre todo o arquivo fonte de uma única vez, produzindo uma lista de tokens. Esta escolha evita I/O frequente e simplifica o tratamento de posições (linha e coluna) de cada token.
2. **Parser**: consome a lista de tokens e constrói uma Árvore de Sintaxe Abstrata (AST). A gramática implementada aceita declarações de funções e variáveis globais, além de estruturas de controle básicas.
3. **Sema (analisador semântico)**: percorre a AST validando tipos e declarações. Também anota cada nó com seu tipo para uso posterior no gerador de código.
4. **Code Generator**: etapa planejada que transformará a AST anotada em código assembly ARM.

Cada fase é isolada em um módulo próprio e pode ser executada individualmente através do programa `mycc` (ver `src/main.c`).

## Etapas planejadas de produção

### Fase 1 – Básico
- Tipos primitivos `int`.
- Controle de fluxo: `if`, `else`, `for`, `while`.
- Variáveis globais e funções com até quatro argumentos.
- Lexer escaneando todo o arquivo em memória.
- Parser produzindo AST e analisador semântico mínimo.

### Fase 2 – Sofisticação
- Novos tipos: `void`, `float` e ponteiros.
- Verificações semânticas completas de tipos e de aridade de funções.
- Expansão da gramática para suportar mais expressões e operações.

### Fase 3 – Autocompilação
- Evoluir o gerador de código para produzir executáveis capazes de compilar o próprio compilador.
- Refino contínuo do front-end e do back-end até atingir auto-hospedagem.

Esta organização incremental permite validar cada estágio do compilador antes de avançar para o próximo. Consulte os arquivos nas pastas `src/` e `tests/` para exemplos de uso.
