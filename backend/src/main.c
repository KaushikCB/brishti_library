#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define MKDIR_PORTABLE(dir) CreateDirectoryA(dir, NULL)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR_PORTABLE(dir) mkdir(dir, 0755)
#endif

int main(int argc, char *argv[]) {
    int port = 8080;
    const char *env_port = getenv("PORT");
    if (env_port && atoi(env_port) > 0) {
        port = atoi(env_port);
    } else if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0) port = 8080;
    }

    /* Ensure database directory exists */
    MKDIR_PORTABLE("database");

    ServerConfig config;
    config.port = port;
    config.web_root = "frontend";
    config.db_path = "database/brishti.db";

    printf("Starting BrishtiLibrary server...\n");
    return server_start(config);
}
