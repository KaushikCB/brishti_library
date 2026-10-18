CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Iinclude
SRC = src/algorithms/search.c src/algorithms/sort.c src/algorithms/greedy.c src/algorithms/knapsack.c src/algorithms/dijkstra.c
TEST_SRC = tests/test_algorithms.c $(SRC)
TEST_BIN = tests/test_algorithms.exe

all: test

test: $(TEST_BIN)
	$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) include/algorithms.h
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_BIN)

clean:
	-del /f /q $(TEST_BIN) *.o
