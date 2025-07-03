CC = gcc
CFLAGS = -Wall -Wextra -Iinc
SRC = $(wildcard src/*.c tests/*.c)
OUT = test

all: test

test: $(SRC)
	$(CC) $(CFLAGS) $(filter-out tests/unity.c,$(SRC)) tests/unity.c -o $(OUT)
	./$(OUT)

gdb: test
	gdb ./$(OUT)

gdb-test: test
	echo "quit" | gdb -q ./$(OUT) > /dev/null 2>&1

clean:
	rm -f $(OUT)
