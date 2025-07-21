CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g -O0 -Iinclude
SRC_LEX = src/lexer/lexer.c
SRC_MAIN= src/main.c
OBJ_LEX = $(SRC_LEX:.c=.o)
OBJ_MAIN= $(SRC_MAIN:.c=.o)

mycc: $(OBJ_LEX) $(OBJ_MAIN)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean test
clean:
	rm -f src/lexer/*.o src/main.o mycc

# --------------------
# Testes de lexer
# --------------------
test-lexer: mycc
	./mycc -tokens tests/lexer/test.c