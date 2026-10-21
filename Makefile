CC ?= gcc
CFLAGS ?= -std=c99 -Wall -Wextra -O2 -Ibackend/include

# Detect OS
ifeq ($(OS),Windows_NT)
    TARGET = backend/server.exe
    TEST_TARGET = backend/tests/test_algorithms.exe
    LDFLAGS = -lws2_32
    RM = del /f /q
else
    TARGET = brishti_server
    TEST_TARGET = backend/tests/test_algorithms
    LDFLAGS = -lpthread -ldl
    RM = rm -f
endif

SRCS = backend/src/main.c \
       backend/src/server.c \
       backend/src/database.c \
       backend/src/algorithm_api.c \
       backend/src/cJSON.c \
       backend/src/sha256.c \
       backend/src/sqlite3.c \
       backend/src/algorithms/search.c \
       backend/src/algorithms/sort.c \
       backend/src/algorithms/greedy.c \
       backend/src/algorithms/knapsack.c \
       backend/src/algorithms/dijkstra.c

ALGO_SRCS = backend/src/algorithms/search.c \
            backend/src/algorithms/sort.c \
            backend/src/algorithms/greedy.c \
            backend/src/algorithms/knapsack.c \
            backend/src/algorithms/dijkstra.c

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): backend/tests/test_algorithms.c $(ALGO_SRCS)
	$(CC) $(CFLAGS) backend/tests/test_algorithms.c $(ALGO_SRCS) -o $(TEST_TARGET)

clean:
	-$(RM) $(TARGET) $(TEST_TARGET) backend/src/*.o
