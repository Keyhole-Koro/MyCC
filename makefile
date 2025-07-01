CC = gcc
CFLAGS = -Wall -Wextra -I./src -I./tests -I./inc
SRC = $(wildcard src/*.c tests/*.c)
OUT = lexer_test

all: test

test: $(SRC)
	$(CC) $(CFLAGS) src/lexer.c tests/unity.c tests/lexer_test.c -o $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)