CC = gcc
CFLAGS = -Wall -Wextra -Iinc
SRC = $(wildcard src/*.c tests/*.c)
OUT = lexer_test

all: test

test: $(SRC)
	$(CC) $(CFLAGS) $(filter-out tests/unity.c,$(SRC)) tests/unity.c -o $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)