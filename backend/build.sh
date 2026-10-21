#!/bin/sh
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "==> Compiling CSE214 Algorithm Test Suite..."
gcc -std=c99 -Wall -Wextra -O2 \
    -I"$SCRIPT_DIR/include" \
    "$SCRIPT_DIR/src/algorithms/search.c" \
    "$SCRIPT_DIR/src/algorithms/sort.c" \
    "$SCRIPT_DIR/src/algorithms/greedy.c" \
    "$SCRIPT_DIR/src/algorithms/knapsack.c" \
    "$SCRIPT_DIR/src/algorithms/dijkstra.c" \
    "$SCRIPT_DIR/tests/test_algorithms.c" \
    -o "$SCRIPT_DIR/tests/test_algorithms"

echo "==> Running CSE214 Test Suite..."
"$SCRIPT_DIR/tests/test_algorithms"

echo "==> Compiling BrishtiLibrary Server Binary for Linux..."
gcc -std=c99 -Wall -Wextra -O2 \
    -I"$SCRIPT_DIR/include" \
    "$SCRIPT_DIR/src/main.c" \
    "$SCRIPT_DIR/src/server.c" \
    "$SCRIPT_DIR/src/database.c" \
    "$SCRIPT_DIR/src/algorithm_api.c" \
    "$SCRIPT_DIR/src/cJSON.c" \
    "$SCRIPT_DIR/src/sha256.c" \
    "$SCRIPT_DIR/src/sqlite3.c" \
    "$SCRIPT_DIR/src/algorithms/"*.c \
    -lpthread -ldl \
    -o "$ROOT_DIR/brishti_server"

echo "==> Build complete: $ROOT_DIR/brishti_server"
