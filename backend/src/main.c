#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main(int argc, char *argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0) port = 8080;
    }

    /* Ensure database directory exists */
    CreateDirectoryA("database", NULL);

    ServerConfig config;
    config.port = port;
    config.web_root = "frontend";
    config.db_path = "database/brishti.db";

    printf("Starting BrishtiLibrary server...\n");
    return server_start(config);
}
