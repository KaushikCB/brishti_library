#include "../include/server.h"
#include "../include/database.h"
#include "../include/algorithm_api.h"
#include "../include/cJSON.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQ_BUF_SIZE 65536
#define RESP_HEADER_SIZE 2048

static const char *get_content_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (strcmp(dot, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".css") == 0) return "text/css; charset=utf-8";
    if (strcmp(dot, ".js") == 0) return "application/javascript; charset=utf-8";
    if (strcmp(dot, ".json") == 0) return "application/json; charset=utf-8";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";
    return "text/plain; charset=utf-8";
}

static void send_http_response(SOCKET sock, int status_code, const char *status_text,
                               const char *content_type, const char *body, size_t body_len,
                               const char *extra_headers) {
    char header_buf[RESP_HEADER_SIZE];
    int header_len = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization, Cookie\r\n"
        "%s"
        "\r\n",
        status_code, status_text, content_type, (unsigned long)body_len,
        extra_headers ? extra_headers : "");

    send(sock, header_buf, header_len, 0);
    if (body && body_len > 0) {
        send(sock, body, (int)body_len, 0);
    }
}

static void send_json_response(SOCKET sock, int status_code, const char *json_str, const char *extra_headers) {
    if (!json_str) json_str = "{}";
    send_http_response(sock, status_code, "OK", "application/json; charset=utf-8", json_str, strlen(json_str), extra_headers);
}

static void send_error_json(SOCKET sock, int status_code, const char *msg) {
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "status", "error");
    cJSON_AddStringToObject(obj, "message", msg);
    char *s = cJSON_PrintUnformatted(obj);
    send_json_response(sock, status_code, s, NULL);
    cJSON_Delete(obj);
    free(s);
}

static void extract_cookie(const char *headers, const char *cookie_name, char *out_val, size_t max_len) {
    out_val[0] = '\0';
    const char *cookie_hdr = strstr(headers, "Cookie:");
    if (!cookie_hdr) cookie_hdr = strstr(headers, "cookie:");
    if (!cookie_hdr) return;

    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "%s=", cookie_name);
    const char *pos = strstr(cookie_hdr, search_pattern);
    if (!pos) return;

    pos += strlen(search_pattern);
    size_t i = 0;
    while (*pos && *pos != ';' && *pos != '\r' && *pos != '\n' && i < max_len - 1) {
        out_val[i++] = *pos++;
    }
    out_val[i] = '\0';
}

static void serve_static_file(SOCKET sock, const char *web_root, const char *url_path) {
    /* Prevent path traversal attacks */
    if (strstr(url_path, "..") || strstr(url_path, "\\")) {
        send_error_json(sock, 403, "Forbidden path");
        return;
    }

    char file_path[512];
    if (strcmp(url_path, "/") == 0 || strcmp(url_path, "/home") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/index.html", web_root);
    } else if (strcmp(url_path, "/algorithms") == 0 || strcmp(url_path, "/algorithms.html") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/pages/algorithms.html", web_root);
    } else if (strcmp(url_path, "/admin") == 0 || strcmp(url_path, "/admin.html") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/pages/admin.html", web_root);
    } else {
        snprintf(file_path, sizeof(file_path), "%s%s", web_root, url_path);
    }

    FILE *f = fopen(file_path, "rb");
    if (!f) {
        send_error_json(sock, 404, "File not found");
        return;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = (char *)malloc(file_size);
    if (!buf) {
        fclose(f);
        send_error_json(sock, 500, "Out of memory");
        return;
    }

    size_t bytes_read = fread(buf, 1, file_size, f);
    fclose(f);

    const char *ctype = get_content_type(file_path);
    send_http_response(sock, 200, "OK", ctype, buf, bytes_read, NULL);
    free(buf);
}

/* Parse query parameters */
static void get_query_param(const char *query, const char *key, char *out_val, size_t max_len) {
    out_val[0] = '\0';
    if (!query || strlen(query) == 0) return;

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "%s=", key);
    const char *pos = strstr(query, pattern);
    if (!pos) return;

    pos += strlen(pattern);
    size_t i = 0;
    while (*pos && *pos != '&' && *pos != ' ' && i < max_len - 1) {
        if (*pos == '+') {
            out_val[i++] = ' ';
        } else if (*pos == '%' && *(pos + 1) && *(pos + 2)) {
            char hex[3] = {*(pos + 1), *(pos + 2), '\0'};
            out_val[i++] = (char)strtol(hex, NULL, 16);
            pos += 2;
        } else {
            out_val[i++] = *pos;
        }
        pos++;
    }
    out_val[i] = '\0';
}

static void handle_client(SOCKET sock, const char *web_root) {
    char req_buf[REQ_BUF_SIZE];
    int total_read = recv(sock, req_buf, sizeof(req_buf) - 1, 0);
    if (total_read <= 0) {
        closesocket(sock);
        return;
    }
    req_buf[total_read] = '\0';

    /* Parse request line */
    char method[16] = "";
    char url[256] = "";
    char version[16] = "";
    sscanf(req_buf, "%15s %255s %15s", method, url, version);

    /* Split URL and query string */
    char path[256] = "";
    char query[256] = "";
    char *qmark = strchr(url, '?');
    if (qmark) {
        size_t path_len = qmark - url;
        strncpy(path, url, path_len);
        path[path_len] = '\0';
        strncpy(query, qmark + 1, sizeof(query) - 1);
    } else {
        strncpy(path, url, sizeof(path) - 1);
    }

    /* Parse Content-Length header if present */
    int content_length = 0;
    const char *cl_hdr = strstr(req_buf, "Content-Length:");
    if (!cl_hdr) cl_hdr = strstr(req_buf, "content-length:");
    if (cl_hdr) {
        content_length = atoi(cl_hdr + 15);
    }

    /* Locate HTTP body and ensure complete payload is received */
    char *body = strstr(req_buf, "\r\n\r\n");
    if (body) {
        body += 4;
        int body_in_buf = total_read - (int)(body - req_buf);
        while (body_in_buf < content_length && total_read < REQ_BUF_SIZE - 1) {
            int n = recv(sock, req_buf + total_read, (REQ_BUF_SIZE - 1) - total_read, 0);
            if (n <= 0) break;
            total_read += n;
            body_in_buf += n;
        }
        req_buf[total_read] = '\0';
    } else {
        body = "";
    }

    /* Extract session cookie */
    char session_token[128] = "";
    extract_cookie(req_buf, "bl_session", session_token, sizeof(session_token));

    char user_name[64] = "";
    char user_email[64] = "";
    char user_role[32] = "";
    int current_user_id = db_validate_session(session_token, user_name, user_email, user_role);

    /* Handle OPTIONS (CORS preflight) */
    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(sock, 204, "No Content", "text/plain", "", 0, NULL);
        closesocket(sock);
        return;
    }

    /* --- ALGORITHM API ENDPOINTS --- */
    if (strcmp(path, "/api/algorithms/search/linear") == 0) {
        char *json = handle_api_linear_search(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/algorithms/search/binary") == 0) {
        char *json = handle_api_binary_search(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/algorithms/sort") == 0) {
        char *json = handle_api_sort(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/algorithms/greedy") == 0) {
        char *json = handle_api_greedy(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/algorithms/dynamic-programming/knapsack") == 0) {
        char *json = handle_api_knapsack(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/algorithms/graph/shortest-path") == 0) {
        char *json = handle_api_dijkstra(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/algorithms/benchmark") == 0) {
        char *json = handle_api_benchmark(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } 

    /* --- STORE PRODUCTS ENDPOINTS --- */
    else if (strcmp(path, "/api/products") == 0) {
        char cat[64] = "";
        char q[64] = "";
        char sort[32] = "";
        char type[32] = "";
        char avail[32] = "";
        char min_p[32] = "";
        char max_p[32] = "";
        char limit_s[32] = "";
        char offset_s[32] = "";

        get_query_param(query, "category", cat, sizeof(cat));
        get_query_param(query, "q", q, sizeof(q));
        get_query_param(query, "sort", sort, sizeof(sort));
        get_query_param(query, "type", type, sizeof(type));
        get_query_param(query, "availability", avail, sizeof(avail));
        get_query_param(query, "min_price", min_p, sizeof(min_p));
        get_query_param(query, "max_price", max_p, sizeof(max_p));
        get_query_param(query, "limit", limit_s, sizeof(limit_s));
        get_query_param(query, "offset", offset_s, sizeof(offset_s));

        double min_price = atof(min_p);
        double max_price = atof(max_p);
        int limit = atoi(limit_s);
        int offset = atoi(offset_s);

        char *json = db_get_products_json(cat, q, sort, type, avail, min_price, max_price, limit, offset);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strncmp(path, "/api/products/", 14) == 0) {
        int pid = atoi(path + 14);
        char *json = db_get_product_by_id_json(pid);
        if (json) {
            send_json_response(sock, 200, json, NULL);
            free(json);
        } else {
            send_error_json(sock, 404, "Product not found");
        }
    } else if (strcmp(path, "/api/categories") == 0) {
        char *json = db_get_categories_json();
        send_json_response(sock, 200, json, NULL);
        free(json);
    }

    /* --- CART & ORDERS ENDPOINTS --- */
    else if (strcmp(path, "/api/cart/validate") == 0) {
        char *json = db_validate_cart_json(body);
        send_json_response(sock, 200, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/orders") == 0 && strcmp(method, "POST") == 0) {
        int order_user = current_user_id > 0 ? current_user_id : 1; /* Default to customer demo if not signed in */
        char *json = db_create_order_json(order_user, body);
        send_json_response(sock, 201, json, NULL);
        free(json);
    } else if (strcmp(path, "/api/orders") == 0 && strcmp(method, "GET") == 0) {
        int order_user = current_user_id > 0 ? current_user_id : 1;
        char *json = db_get_user_orders_json(order_user);
        send_json_response(sock, 200, json, NULL);
        free(json);
    }

    /* --- AUTHENTICATION ENDPOINTS --- */
    else if (strcmp(path, "/api/auth/me") == 0) {
        cJSON *me = cJSON_CreateObject();
        if (current_user_id > 0) {
            cJSON_AddBoolToObject(me, "authenticated", true);
            cJSON_AddNumberToObject(me, "id", current_user_id);
            cJSON_AddStringToObject(me, "name", user_name);
            cJSON_AddStringToObject(me, "email", user_email);
            cJSON_AddStringToObject(me, "role", user_role);
        } else {
            cJSON_AddBoolToObject(me, "authenticated", false);
        }
        char *s = cJSON_PrintUnformatted(me);
        send_json_response(sock, 200, s, NULL);
        cJSON_Delete(me);
        free(s);
    } else if (strcmp(path, "/api/auth/demo-login") == 0) {
        cJSON *req = cJSON_Parse(body);
        const char *role_req = "customer";
        if (req) {
            cJSON *r = cJSON_GetObjectItem(req, "role");
            if (r && r->valuestring) role_req = r->valuestring;
        }

        int uid = 1;
        if (strcmp(role_req, "admin") == 0) uid = 2;

        char *token = db_create_session(uid);
        char cookie_hdr[256];
        snprintf(cookie_hdr, sizeof(cookie_hdr), "Set-Cookie: bl_session=%s; Path=/; HttpOnly; SameSite=Lax\r\n", token);

        cJSON *resp = cJSON_CreateObject();
        cJSON_AddStringToObject(resp, "status", "success");
        cJSON_AddStringToObject(resp, "token", token);
        cJSON_AddStringToObject(resp, "role", (uid == 2) ? "admin" : "customer");
        cJSON_AddStringToObject(resp, "message", (uid == 2) ? "Signed in as Brishti Admin" : "Signed in as CSE214 Student Demo");

        char *s = cJSON_PrintUnformatted(resp);
        send_json_response(sock, 200, s, cookie_hdr);
        cJSON_Delete(resp);
        if (req) cJSON_Delete(req);
        free(token);
        free(s);
    } else if (strcmp(path, "/api/auth/google") == 0) {
        /* Google sign-in endpoint: parses credential id_token or development token */
        cJSON *req = cJSON_Parse(body);
        const char *id_token = "";
        const char *dev_email = "student@brishtilibrary.edu";
        const char *dev_name = "CSE214 Google User";
        if (req) {
            cJSON *t = cJSON_GetObjectItem(req, "credential");
            cJSON *em = cJSON_GetObjectItem(req, "email");
            cJSON *nm = cJSON_GetObjectItem(req, "name");
            if (t && t->valuestring) id_token = t->valuestring;
            if (em && em->valuestring) dev_email = em->valuestring;
            if (nm && nm->valuestring) dev_name = nm->valuestring;
        }

        int uid = db_find_or_create_user(dev_email, dev_name, id_token, "customer");
        char *token = db_create_session(uid);
        char cookie_hdr[256];
        snprintf(cookie_hdr, sizeof(cookie_hdr), "Set-Cookie: bl_session=%s; Path=/; HttpOnly; SameSite=Lax\r\n", token);

        cJSON *resp = cJSON_CreateObject();
        cJSON_AddStringToObject(resp, "status", "success");
        cJSON_AddStringToObject(resp, "token", token);
        cJSON_AddStringToObject(resp, "name", dev_name);
        cJSON_AddStringToObject(resp, "email", dev_email);
        cJSON_AddStringToObject(resp, "role", "customer");
        char *s = cJSON_PrintUnformatted(resp);
        send_json_response(sock, 200, s, cookie_hdr);
        cJSON_Delete(resp);
        if (req) cJSON_Delete(req);
        free(token);
        free(s);
    } else if (strcmp(path, "/api/auth/logout") == 0) {
        db_delete_session(session_token);
        char cookie_hdr[] = "Set-Cookie: bl_session=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n";
        send_json_response(sock, 200, "{\"status\":\"success\",\"message\":\"Logged out\"}", cookie_hdr);
    }

    /* --- ADMIN ENDPOINTS (Protected: requires admin role) --- */
    else if (strncmp(path, "/api/admin/", 11) == 0) {
        if (current_user_id <= 0 || strcmp(user_role, "admin") != 0) {
            send_error_json(sock, 403, "Admin authorization required");
            closesocket(sock);
            return;
        }

        if (strcmp(path, "/api/admin/orders") == 0 && strcmp(method, "GET") == 0) {
            char *json = db_admin_get_all_orders_json();
            send_json_response(sock, 200, json, NULL);
            free(json);
        } else if (strncmp(path, "/api/admin/orders/", 18) == 0 && (strcmp(method, "PATCH") == 0 || strcmp(method, "PUT") == 0)) {
            int oid = atoi(path + 18);
            cJSON *req = cJSON_Parse(body);
            const char *new_status = "Confirmed";
            if (req) {
                cJSON *s = cJSON_GetObjectItem(req, "status");
                if (s && s->valuestring) new_status = s->valuestring;
            }
            char *json = db_admin_update_order_status_json(oid, new_status);
            send_json_response(sock, 200, json, NULL);
            if (req) cJSON_Delete(req);
            free(json);
        } else if (strcmp(path, "/api/admin/products") == 0 && strcmp(method, "GET") == 0) {
            char *json = db_admin_get_all_products_json();
            send_json_response(sock, 200, json, NULL);
            free(json);
        } else if (strcmp(path, "/api/admin/products") == 0 && strcmp(method, "POST") == 0) {
            char *json = db_admin_create_product_json(body);
            int code = (strstr(json, "\"status\":\"error\"") != NULL) ? 400 : 201;
            send_json_response(sock, code, json, NULL);
            free(json);
        } else if (strncmp(path, "/api/admin/products/", 20) == 0 && (strcmp(method, "PUT") == 0 || strcmp(method, "PATCH") == 0)) {
            int pid = atoi(path + 20);
            char *json = db_admin_update_product_json(pid, body);
            int code = 200;
            if (strstr(json, "\"status\":\"error\"") != NULL) {
                if (strstr(json, "not found") != NULL) {
                    code = 404;
                } else {
                    code = 400;
                }
            }
            send_json_response(sock, code, json, NULL);
            free(json);
        } else if (strncmp(path, "/api/admin/products/", 20) == 0 && strcmp(method, "DELETE") == 0) {
            int pid = atoi(path + 20);
            char *json = db_admin_delete_product_json(pid);
            int code = 200;
            if (strstr(json, "\"status\":\"error\"") != NULL) {
                if (strstr(json, "not found") != NULL) {
                    code = 404;
                } else {
                    code = 400;
                }
            }
            send_json_response(sock, code, json, NULL);
            free(json);
        } else {
            send_error_json(sock, 404, "Admin endpoint not found");
        }
    }

    /* --- STATIC FILE SERVING --- */
    else {
        serve_static_file(sock, web_root, path);
    }

    closesocket(sock);
}

int server_start(ServerConfig config) {
    WSADATA wsa;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (rc != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", rc);
        return 1;
    }

    /* Initialize Database */
    rc = db_init(config.db_path);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to initialize SQLite database at %s\n", config.db_path);
        WSACleanup();
        return 1;
    }

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        db_close();
        WSACleanup();
        return 1;
    }

    /* Allow address reuse */
    BOOL opt = TRUE;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(config.port);

    if (bind(listen_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind to port %d failed: %d\n", config.port, WSAGetLastError());
        closesocket(listen_sock);
        db_close();
        WSACleanup();
        return 1;
    }

    if (listen(listen_sock, 16) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        db_close();
        WSACleanup();
        return 1;
    }

    printf("====================================================\n");
    printf("   BrishtiLibrary C Backend Server (CSE214)\n");
    printf("   Port: %d\n", config.port);
    printf("   Web Root: %s\n", config.web_root);
    printf("   Database: %s\n", config.db_path);
    printf("   URL: http://localhost:%d\n", config.port);
    printf("====================================================\n");
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == INVALID_SOCKET) {
            continue;
        }
        handle_client(client_sock, config.web_root);
    }

    closesocket(listen_sock);
    db_close();
    WSACleanup();
    return 0;
}
