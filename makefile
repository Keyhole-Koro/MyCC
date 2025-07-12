CC = gcc
CFLAGS = -Wall -Wextra -Iinc
SRC = $(wildcard src/*.c)
TESTS = $(wildcard tests/*.c)
SRC_NO_MAIN = $(filter-out src/main.c, $(SRC))
OUT = test
MYCC = mycc

all: test

test: $(SRC_NO_MAIN) $(TESTS)
	$(CC) $(CFLAGS) -g $(SRC_NO_MAIN) $(TESTS) -o $(OUT)
	./$(OUT)

debug: $(SRC_NO_MAIN) $(TESTS)
	$(CC) $(CFLAGS) -g $(SRC_NO_MAIN) $(TESTS) -o $(OUT)
	gdb ./$(OUT)

run-mycc: $(SRC)
	$(CC) $(CFLAGS) -o $(MYCC) $(SRC)
	./$(MYCC) $(IN) $(OUT)

debug-mycc: $(SRC)
	$(CC) $(CFLAGS) -g -o $(MYCC) $(SRC)
	gdb --args ./$(MYCC) $(IN) $(OUT)


clean:
	rm -f $(OUT) $(MYCC)
