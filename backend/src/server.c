#include "../include/server.h"
#include "../include/database.h"
#include "../include/algorithm_api.h"
#include "../include/cJSON.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#define MKDIR_PORTABLE(dir) CreateDirectoryA(dir, NULL)
#define LAST_SOCK_ERR() WSAGetLastError()
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket(s) close(s)
#define MKDIR_PORTABLE(dir) mkdir(dir, 0755)
#define LAST_SOCK_ERR() errno
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

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
    if (strcmp(dot, ".webp") == 0) return "image/webp";
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

/* Base64 & Secure Image Upload Helpers */
static int b64_char_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static uint8_t *b64_decode_buffer(const char *in, size_t in_len, size_t *out_len) {
    if (!in || in_len == 0) return NULL;
    const char *comma = strchr(in, ',');
    if (comma && (comma - in) < 80) {
        in = comma + 1;
        in_len = strlen(in);
    }
    uint8_t *out = (uint8_t *)malloc((in_len * 3) / 4 + 16);
    if (!out) return NULL;
    size_t o = 0;
    int buf = 0, bits = 0;
    for (size_t i = 0; i < in_len; i++) {
        char c = in[i];
        if (c == '=' || c == '\r' || c == '\n' || c == ' ' || c == '"') continue;
        int v = b64_char_val(c);
        if (v < 0) continue;
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[o++] = (uint8_t)((buf >> bits) & 0xFF);
        }
    }
    *out_len = o;
    return out;
}

static void handle_image_upload(SOCKET sock, const char *web_root, const char *body) {
    cJSON *root = cJSON_Parse(body);
    if (!root) {
        send_error_json(sock, 400, "Invalid JSON in upload request");
        return;
    }

    cJSON *data_item = cJSON_GetObjectItem(root, "data");
    if (!data_item || !data_item->valuestring || strlen(data_item->valuestring) == 0) {
        cJSON_Delete(root);
        send_error_json(sock, 400, "Image data is required");
        return;
    }

    size_t byte_len = 0;
    uint8_t *bytes = b64_decode_buffer(data_item->valuestring, strlen(data_item->valuestring), &byte_len);
    cJSON_Delete(root);

    if (!bytes || byte_len < 16) {
        if (bytes) free(bytes);
        send_error_json(sock, 400, "Corrupted or empty image payload");
        return;
    }

    if (byte_len > 5 * 1024 * 1024) {
        free(bytes);
        send_error_json(sock, 400, "Image size exceeds 5MB limit");
        return;
    }

    /* Magic byte validation for JPEG, PNG, WebP */
    const char *ext = NULL;
    if (byte_len >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
        ext = "jpg";
    } else if (byte_len >= 8 && memcmp(bytes, "\x89PNG\r\n\x1a\n", 8) == 0) {
        ext = "png";
    } else if (byte_len >= 12 && memcmp(bytes, "RIFF", 4) == 0 && memcmp(bytes + 8, "WEBP", 4) == 0) {
        ext = "webp";
    }

    if (!ext) {
        free(bytes);
        send_error_json(sock, 400, "Invalid image format. Only JPEG, PNG, and WebP are allowed.");
        return;
    }

    /* Ensure uploads directory exists */
    char upload_dir[512];
    snprintf(upload_dir, sizeof(upload_dir), "%s/uploads", web_root);
    MKDIR_PORTABLE(upload_dir);

    /* Generate safe unique filename preventing path traversal or execution */
    char safe_filename[64];
    snprintf(safe_filename, sizeof(safe_filename), "bl_%lu_%08x.%s", 
             (unsigned long)time(NULL), (unsigned int)rand(), ext);

    char full_dest[512];
    snprintf(full_dest, sizeof(full_dest), "%s/%s", upload_dir, safe_filename);

    FILE *f = fopen(full_dest, "wb");
    if (!f) {
        free(bytes);
        send_error_json(sock, 500, "Failed to save image to disk");
        return;
    }

    fwrite(bytes, 1, byte_len, f);
    fclose(f);
    free(bytes);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Image uploaded successfully");
    char url_buf[128];
    snprintf(url_buf, sizeof(url_buf), "/uploads/%s", safe_filename);
    cJSON_AddStringToObject(resp, "url", url_buf);
    cJSON_AddStringToObject(resp, "filename", safe_filename);
    cJSON_AddNumberToObject(resp, "size", (double)byte_len);

    char *s = cJSON_PrintUnformatted(resp);
    send_json_response(sock, 200, s, NULL);
    cJSON_Delete(resp);
    free(s);
}

static void handle_client(SOCKET sock, const char *web_root) {
    char initial_buf[REQ_BUF_SIZE];
    int total_read = recv(sock, initial_buf, sizeof(initial_buf) - 1, 0);
    if (total_read <= 0) {
        closesocket(sock);
        return;
    }
    initial_buf[total_read] = '\0';

    /* Parse request line */
    char method[16] = "";
    char url[256] = "";
    char version[16] = "";
    sscanf(initial_buf, "%15s %255s %15s", method, url, version);

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
    const char *cl_hdr = strstr(initial_buf, "Content-Length:");
    if (!cl_hdr) cl_hdr = strstr(initial_buf, "content-length:");
    if (cl_hdr) {
        content_length = atoi(cl_hdr + 15);
    }

    if (content_length > 8 * 1024 * 1024) {
        send_error_json(sock, 413, "Payload exceeds maximum allowed size (5MB)");
        closesocket(sock);
        return;
    }

    char *req_buf = initial_buf;
    char *heap_buf = NULL;
    char *body_sep = strstr(initial_buf, "\r\n\r\n");
    int header_bytes = body_sep ? (int)(body_sep + 4 - initial_buf) : total_read;
    int total_expected = header_bytes + content_length;

    if (total_expected >= REQ_BUF_SIZE) {
        heap_buf = (char *)malloc(total_expected + 4096);
        if (heap_buf) {
            memcpy(heap_buf, initial_buf, total_read);
            heap_buf[total_read] = '\0';
            req_buf = heap_buf;
        }
    }

    /* Locate HTTP body and ensure complete payload is received */
    char *body = strstr(req_buf, "\r\n\r\n");
    if (body) {
        body += 4;
        int body_in_buf = total_read - (int)(body - req_buf);
        int max_capacity = heap_buf ? (total_expected + 4000) : (REQ_BUF_SIZE - 1);
        while (body_in_buf < content_length && total_read < max_capacity) {
            int to_read = max_capacity - total_read;
            if (to_read <= 0) break;
            int n = recv(sock, req_buf + total_read, to_read, 0);
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

        /* Strictly forbid demo admin login: Admin requires secure password authentication */
        if (strcmp(role_req, "admin") == 0) {
            send_error_json(sock, 403, "Admin demo login is disabled. Please log in using secure admin credentials.");
            if (req) cJSON_Delete(req);
            if (heap_buf) free(heap_buf);
            closesocket(sock);
            return;
        }

        int uid = 1; /* Customer only */
        char *token = db_create_session(uid);
        char cookie_hdr[256];
        snprintf(cookie_hdr, sizeof(cookie_hdr), "Set-Cookie: bl_session=%s; Path=/; HttpOnly; SameSite=Lax\r\n", token);

        cJSON *resp = cJSON_CreateObject();
        cJSON_AddStringToObject(resp, "status", "success");
        cJSON_AddStringToObject(resp, "token", token);
        cJSON_AddStringToObject(resp, "role", "customer");
        cJSON_AddStringToObject(resp, "message", "Signed in as CSE214 Student Demo");

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

    /* --- PUBLIC ADMIN SETUP & AUTHENTICATION ENDPOINTS --- */
    else if (strcmp(path, "/api/admin/setup-status") == 0 && strcmp(method, "GET") == 0) {
        int done = db_admin_is_setup_completed();
        cJSON *st = cJSON_CreateObject();
        cJSON_AddStringToObject(st, "status", "success");
        cJSON_AddBoolToObject(st, "setup_completed", done ? true : false);
        char *s = cJSON_PrintUnformatted(st);
        send_json_response(sock, 200, s, NULL);
        cJSON_Delete(st);
        free(s);
    } else if (strcmp(path, "/api/admin/setup") == 0 && strcmp(method, "POST") == 0) {
        cJSON *req = cJSON_Parse(body);
        const char *email = "admin";
        const char *pw = "";
        if (req) {
            cJSON *em = cJSON_GetObjectItem(req, "username");
            if (!em) em = cJSON_GetObjectItem(req, "email");
            cJSON *p = cJSON_GetObjectItem(req, "password");
            if (em && em->valuestring && strlen(em->valuestring) > 0) email = em->valuestring;
            if (p && p->valuestring) pw = p->valuestring;
        }
        char *json = db_admin_setup_json(email, pw);
        int code = (strstr(json, "\"status\":\"error\"") != NULL) ? 400 : 200;
        char cookie_hdr[256] = "";
        if (code == 200) {
            cJSON *res_obj = cJSON_Parse(json);
            if (res_obj) {
                cJSON *tok = cJSON_GetObjectItem(res_obj, "token");
                if (tok && tok->valuestring && strlen(tok->valuestring) > 0) {
                    snprintf(cookie_hdr, sizeof(cookie_hdr), "Set-Cookie: bl_session=%s; Path=/; HttpOnly; SameSite=Lax\r\n", tok->valuestring);
                }
                cJSON_Delete(res_obj);
            }
        }
        send_json_response(sock, code, json, (code == 200 && strlen(cookie_hdr) > 0) ? cookie_hdr : NULL);
        if (req) cJSON_Delete(req);
        free(json);
    } else if (strcmp(path, "/api/admin/login") == 0 && strcmp(method, "POST") == 0) {
        cJSON *req = cJSON_Parse(body);
        const char *email = "";
        const char *pw = "";
        if (req) {
            cJSON *em = cJSON_GetObjectItem(req, "username");
            if (!em) em = cJSON_GetObjectItem(req, "email");
            cJSON *p = cJSON_GetObjectItem(req, "password");
            if (em && em->valuestring) email = em->valuestring;
            if (p && p->valuestring) pw = p->valuestring;
        }
        char token_buf[128] = "";
        char *json = db_admin_login_json(email, pw, token_buf, sizeof(token_buf));
        int code = 200;
        if (strstr(json, "\"status\":\"error\"") != NULL) {
            if (strstr(json, "Invalid admin credentials") != NULL) code = 401;
            else code = 400;
        }
        char cookie_hdr[256] = "";
        if (code == 200 && strlen(token_buf) > 0) {
            snprintf(cookie_hdr, sizeof(cookie_hdr), "Set-Cookie: bl_session=%s; Path=/; HttpOnly; SameSite=Lax\r\n", token_buf);
        }
        send_json_response(sock, code, json, (code == 200 && strlen(cookie_hdr) > 0) ? cookie_hdr : NULL);
        if (req) cJSON_Delete(req);
        free(json);
    }

    /* --- PROTECTED ADMIN ENDPOINTS (Strictly requires admin role) --- */
    else if (strncmp(path, "/api/admin/", 11) == 0) {
        if (current_user_id <= 0 || strcmp(user_role, "admin") != 0) {
            send_error_json(sock, 403, "Admin authorization required");
            if (heap_buf) free(heap_buf);
            closesocket(sock);
            return;
        }

        if (strcmp(path, "/api/admin/change-password") == 0 && strcmp(method, "POST") == 0) {
            cJSON *req = cJSON_Parse(body);
            const char *cur_pw = "";
            const char *new_pw = "";
            if (req) {
                cJSON *cp = cJSON_GetObjectItem(req, "current_password");
                cJSON *np = cJSON_GetObjectItem(req, "new_password");
                if (cp && cp->valuestring) cur_pw = cp->valuestring;
                if (np && np->valuestring) new_pw = np->valuestring;
            }
            char *json = db_admin_change_password_json(current_user_id, cur_pw, new_pw);
            int code = (strstr(json, "\"status\":\"error\"") != NULL) ? 400 : 200;
            send_json_response(sock, code, json, NULL);
            if (req) cJSON_Delete(req);
            free(json);
        } else if (strcmp(path, "/api/admin/upload") == 0 && strcmp(method, "POST") == 0) {
            handle_image_upload(sock, web_root, body);
        } else if (strcmp(path, "/api/admin/backup") == 0 && strcmp(method, "POST") == 0) {
            cJSON *req = cJSON_Parse(body);
            const char *custom_name = NULL;
            if (req) {
                cJSON *fn = cJSON_GetObjectItem(req, "filename");
                if (fn && fn->valuestring && strlen(fn->valuestring) > 0) custom_name = fn->valuestring;
            }
            char *json = db_admin_create_backup_json(custom_name);
            int code = (strstr(json, "\"status\":\"error\"") != NULL) ? 500 : 201;
            send_json_response(sock, code, json, NULL);
            if (req) cJSON_Delete(req);
            free(json);
        } else if (strcmp(path, "/api/admin/backups") == 0 && strcmp(method, "GET") == 0) {
            char *json = db_admin_list_backups_json();
            send_json_response(sock, 200, json, NULL);
            free(json);
        } else if (strncmp(path, "/api/admin/backups/download/", 28) == 0 && strcmp(method, "GET") == 0) {
            const char *filename = path + 28;
            if (strstr(filename, "..") || strchr(filename, '/') || strchr(filename, '\\')) {
                send_error_json(sock, 403, "Forbidden path");
            } else {
                char full_dest[512];
                snprintf(full_dest, sizeof(full_dest), "database/backups/%s", filename);
                FILE *bf = fopen(full_dest, "rb");
                if (!bf) {
                    send_error_json(sock, 404, "Backup file not found");
                } else {
                    fseek(bf, 0, SEEK_END);
                    long sz = ftell(bf);
                    fseek(bf, 0, SEEK_SET);
                    char *buf = (char *)malloc(sz);
                    if (buf) {
                        fread(buf, 1, sz, bf);
                        char disp_hdr[256];
                        snprintf(disp_hdr, sizeof(disp_hdr), "Content-Disposition: attachment; filename=\"%s\"\r\n", filename);
                        send_http_response(sock, 200, "OK", "application/x-sqlite3", buf, sz, disp_hdr);
                        free(buf);
                    } else {
                        send_error_json(sock, 500, "Out of memory");
                    }
                    fclose(bf);
                }
            }
        } else if (strncmp(path, "/api/admin/backups/", 19) == 0 && strcmp(method, "DELETE") == 0) {
            const char *filename = path + 19;
            int rc = db_admin_delete_backup(filename);
            if (rc == 0) {
                send_json_response(sock, 200, "{\"status\":\"success\",\"message\":\"Backup deleted successfully\"}", NULL);
            } else {
                send_error_json(sock, 400, "Failed to delete backup file or file not found");
            }
        } else if (strcmp(path, "/api/admin/categories") == 0 && strcmp(method, "POST") == 0) {
            char *json = db_admin_create_category_json(body);
            int code = (strstr(json, "\"status\":\"error\"") != NULL) ? 400 : 201;
            send_json_response(sock, code, json, NULL);
            free(json);
        } else if (strncmp(path, "/api/admin/categories/", 22) == 0 && (strcmp(method, "PUT") == 0 || strcmp(method, "PATCH") == 0)) {
            int cid = atoi(path + 22);
            char *json = db_admin_update_category_json(cid, body);
            int code = (strstr(json, "\"status\":\"error\"") != NULL) ? 400 : 200;
            send_json_response(sock, code, json, NULL);
            free(json);
        } else if (strncmp(path, "/api/admin/categories/", 22) == 0 && strcmp(method, "DELETE") == 0) {
            int cid = atoi(path + 22);
            char *json = db_admin_delete_category_json(cid);
            int code = (strstr(json, "\"status\":\"error\"") != NULL) ? 400 : 200;
            send_json_response(sock, code, json, NULL);
            free(json);
        } else if (strcmp(path, "/api/admin/orders") == 0 && strcmp(method, "GET") == 0) {
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

    if (heap_buf) free(heap_buf);
    closesocket(sock);
}

int server_start(ServerConfig config) {
#ifdef _WIN32
    WSADATA wsa;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (rc != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", rc);
        return 1;
    }
#endif

    /* Initialize Database */
    int db_rc = db_init(config.db_path);
    if (db_rc != SQLITE_OK) {
        fprintf(stderr, "Failed to initialize SQLite database at %s\n", config.db_path);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    /* Ensure uploads directory exists */
    char upload_dir[512];
    snprintf(upload_dir, sizeof(upload_dir), "%s/uploads", config.web_root);
    MKDIR_PORTABLE(upload_dir);

    /* Ensure database backups directory exists */
    MKDIR_PORTABLE("database");
    MKDIR_PORTABLE("database/backups");

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", LAST_SOCK_ERR());
        db_close();
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    /* Allow address reuse */
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(config.port);

    if (bind(listen_sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind to port %d failed: %d\n", config.port, LAST_SOCK_ERR());
        closesocket(listen_sock);
        db_close();
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(listen_sock, 16) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed: %d\n", LAST_SOCK_ERR());
        closesocket(listen_sock);
        db_close();
#ifdef _WIN32
        WSACleanup();
#endif
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
        socklen_t client_len = sizeof(client_addr);
        SOCKET client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == INVALID_SOCKET) {
            continue;
        }
        handle_client(client_sock, config.web_root);
    }

    closesocket(listen_sock);
    db_close();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
