CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g -O0 -Iinclude
SRC_LEX = src/lexer/lexer.c
SRC_PRSR  = src/parser/parser.c
SRC_MAIN= src/main.c


OBJ_LEX = $(SRC_LEX:.c=.o)
OBJ_PRSR  = $(SRC_PRSR:.c=.o)
OBJ_MAIN= $(SRC_MAIN:.c=.o)

mycc: $(OBJ_LEX) $(OBJ_PRSR) $(OBJ_MAIN)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean test
clean:
	rm -f src/lexer/*.o src/parser/*.o src/main.o mycc tests/parser/*.got.ast

# --------------------
# Testes de lexer
# --------------------
test-lexer: mycc
	./mycc -tokens tests/lexer/test.c

# --------------------
# Testes de parser
# --------------------
test-parser: mycc
# 	@for f in tests/parser/*.c; do \
# 	  echo "== Test $$f =="; \
# 	  diff -u $${f%.c}.ast $${f%.c}.got.ast || exit 1; \
# 	done
	@for f in tests/parser/*.c; do \
	    echo "== AST $$f =="; \
	  	./mycc -ast $$f > $${f%.c}.got.ast; \
	    ./mycc -ast $$f; \
	done


# alias “test” para rodar tudo
test: test-lexer test-parser