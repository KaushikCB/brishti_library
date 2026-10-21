# Multi-stage Dockerfile for BrishtiLibrary (Pure C99 Backend + SQLite + Static Frontend)

# ==========================================
# Stage 1: Build C binary with GCC on Linux
# ==========================================
FROM alpine:3.19 AS builder

RUN apk add --no-cache gcc musl-dev make

WORKDIR /build

# Copy backend source code
COPY backend/ ./backend/

# Compile standalone C backend
RUN gcc -std=c99 -Wall -Wextra -O2 \
    -Ibackend/include \
    backend/src/main.c \
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
    backend/src/algorithms/dijkstra.c \
    -lpthread -ldl \
    -o brishti_server

# ==========================================
# Stage 2: Minimal runtime image (~15MB)
# ==========================================
FROM alpine:3.19

WORKDIR /app

# Copy compiled executable and frontend assets
COPY --from=builder /build/brishti_server ./
COPY frontend/ ./frontend/

# Ensure runtime directories exist
RUN mkdir -p database database/backups frontend/uploads

# Default port (overridden by Render/Railway/Fly.io via $PORT)
ENV PORT=8080
EXPOSE 8080

CMD ["./brishti_server"]
