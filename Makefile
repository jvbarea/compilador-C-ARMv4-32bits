CC        = gcc
CFLAGS    = -std=c11 -Wall -Wextra -g -O0 -Iinclude
SRC_LEX   = src/lexer/lexer.c
SRC_TYPE  = src/type/type.c
SRC_PRSR  = src/parser/parser.c
SRC_SEMA  = src/sema/sema.c
SRC_CODEGEN = src/code_generator/code_generator.c
SRC_MAIN  = src/main.c


OBJ_LEX   = $(SRC_LEX:.c=.o)
OBJ_TYPE  = $(SRC_TYPE:.c=.o)
OBJ_PRSR  = $(SRC_PRSR:.c=.o)
OBJ_SEMA  = $(SRC_SEMA:.c=.o)
OBJ_CODEGEN = $(SRC_CODEGEN:.c=.o)
OBJ_MAIN  = $(SRC_MAIN:.c=.o)

mycc: $(OBJ_LEX) $(OBJ_PRSR) $(OBJ_SEMA) $(OBJ_TYPE) $(OBJ_CODEGEN) $(OBJ_MAIN)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean test
clean:
	rm -f src/lexer/*.o src/parser/*.o src/sema/*.o src/code_generator/*.o src/main.o tests/parser/*.got.ast tests/sema/*.got.err mycc

# --------------------
# Testes de lexer
# --------------------
test-lexer: mycc
	./mycc -tokens tests/lexer/test.c

# --------------------
# Testes de parser
# --------------------
test-parser: mycc
	@for f in tests/parser/*.c; do \
	    echo "== AST $$f =="; \
	  	./mycc -ast $$f > $${f%.c}.got.ast; \
	    ./mycc -ast $$f; \
	done

# --------------------
# Testes de semantica
# --------------------
test-sema: mycc
# 	@for f in tests/sema/*.c ; do \
# 	   printf "== Sema $$f ==\n"; \
# 	   ./mycc -sema $$f >/dev/null 2>&1 && echo OK || echo "ERRO detectado"; \
# 	 done
test-sema: mycc
	@for f in tests/sema/*.c; do \
	    base=$$(basename $$f .c); \
	    out=tests/sema/$$base.got.err; \
	    printf "== Sema $$f ==\n"; \
	    if ./mycc -sema $$f 2> $$out; then \
	        echo "OK (sem erros)"; \
	        rm $$out; \
	    else \
	        echo "ERROS listados em $$out"; \
	        cat  $$out; \
	    fi; \
	    echo ; \
	done

# alias “test” para rodar tudo
test: test-lexer test-parser test-sema