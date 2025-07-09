CC = gcc
CFLAGS = -Wall -Wextra -Iinc
SRC = $(wildcard src/*.c tests/*.c)
SRC_NO_TEST = $(filter-out tests/%.c, $(SRC))
OUT = test
MYCC = mycc

all: test

test: $(SRC)
	$(CC) $(CFLAGS) $(filter-out tests/unity.c,$(SRC)) tests/unity.c -o $(OUT)
	./$(OUT)

run-mycc: $(SRC_NO_TEST)
	$(CC) $(CFLAGS) -o $(MYCC) $(SRC_NO_TEST)
	./$(MYCC) $(IN) $(OUT)

clean:
	rm -f $(OUT) $(MYCC)
