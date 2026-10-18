#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>

/* Server configuration */
typedef struct {
    int port;
    const char *web_root;
    const char *db_path;
} ServerConfig;

/* Start the C HTTP server (blocks until terminated) */
int server_start(ServerConfig config);

#endif /* SERVER_H */
