#ifndef DATABASE_H
#define DATABASE_H

#include "sqlite3.h"
#include <stdbool.h>

/* Global or managed db handle */
extern sqlite3 *g_db;

/* Database lifecycle */
int db_init(const char *db_file_path);
void db_close(void);

/* Products API */
char *db_get_products_json(const char *category, const char *search_q, const char *sort_by, const char *product_type, const char *availability, double min_price, double max_price, int limit, int offset);
char *db_get_product_by_id_json(int id);
char *db_get_categories_json(void);
char *db_admin_get_all_products_json(void);
char *db_admin_create_product_json(const char *json_body);
char *db_admin_update_product_json(int id, const char *json_body);
char *db_admin_delete_product_json(int id);

/* Cart & Order API */
char *db_validate_cart_json(const char *cart_json_str);
char *db_create_order_json(int user_id, const char *order_json_str);
char *db_get_user_orders_json(int user_id);
char *db_get_order_by_id_json(int order_id, int user_id, bool is_admin);
char *db_admin_get_all_orders_json(void);
char *db_admin_update_order_status_json(int order_id, const char *new_status);

/* Auth & User API */
int db_find_or_create_user(const char *email, const char *name, const char *google_id, const char *role);
char *db_create_session(int user_id);
int db_validate_session(const char *session_token, char *out_name, char *out_email, char *out_role);
void db_delete_session(const char *session_token);

#endif /* DATABASE_H */
