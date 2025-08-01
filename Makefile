CC        = gcc
CFLAGS    = -std=c11 -Wall -Wextra -g -O0 -Iinclude
SRC_LEX   = src/lexer/lexer.c
SRC_TYPE  = src/type/type.c
SRC_PRSR  = src/parser/parser.c
SRC_SEMA  = src/sema/sema.c
SRC_CGEN  = src/code_generator/code_generator.c
SRC_MAIN  = src/main.c


OBJ_LEX   = $(SRC_LEX:.c=.o)
OBJ_TYPE  = $(SRC_TYPE:.c=.o)
OBJ_PRSR  = $(SRC_PRSR:.c=.o)
OBJ_SEMA  = $(SRC_SEMA:.c=.o)
OBJ_CGEN  = $(SRC_CGEN:.c=.o)
OBJ_MAIN  = $(SRC_MAIN:.c=.o)

mycc: $(OBJ_LEX) $(OBJ_PRSR) $(OBJ_SEMA) $(OBJ_TYPE) $(OBJ_CGEN) $(OBJ_MAIN)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# --------------------------------------------------
#  regra genérica: gera test1.s a partir de test1.c
# --------------------------------------------------
%.s: %.c mycc
	./mycc -S $< > $@

.PHONY: clean test
clean:
	rm -f src/lexer/*.o src/type/*.o src/parser/*.o src/sema/*.o src/code_generator/*.o src/main.o tests/parser/*.got.ast tests/sema/*.got.err mycc 

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
 # --------------------
 # Testes de Code Generation
 # --------------------
 test-cgen: mycc
	@for f in tests/code_generator/*.c; do \
	    echo "== ASM $$f =="; \
	    asm=$${f%.c}.got;      \
	    ./mycc -S $$f > $$asm;    \
	    cat $$asm;                \
 	done

# alias “test” para rodar tudo
test: test-lexer test-parser test-sema test-cgen