#include "../include/database.h"
#include "../include/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

sqlite3 *g_db = NULL;

static void generate_random_token(char *out_buf, size_t len) {
    static const char hex_chars[] = "0123456789abcdef";
    for (size_t i = 0; i < len - 1; i++) {
        out_buf[i] = hex_chars[rand() % 16];
    }
    out_buf[len - 1] = '\0';
}

int db_init(const char *db_file_path) {
    if (g_db) return 0;
    srand((unsigned int)time(NULL));

    int rc = sqlite3_open(db_file_path, &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open SQLite database %s: %s\n", db_file_path, sqlite3_errmsg(g_db));
        return rc;
    }

    /* Enable WAL mode and foreign keys */
    sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(g_db, "PRAGMA foreign_keys=ON;", NULL, NULL, NULL);

    /* Schema initialization */
    const char *schema_sql = 
        "CREATE TABLE IF NOT EXISTS products ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sku TEXT UNIQUE NOT NULL,"
        "  type TEXT NOT NULL,"
        "  title TEXT NOT NULL,"
        "  author TEXT,"
        "  isbn TEXT,"
        "  publisher TEXT,"
        "  edition TEXT,"
        "  language TEXT DEFAULT 'English',"
        "  brand TEXT,"
        "  color TEXT,"
        "  size TEXT,"
        "  specifications TEXT,"
        "  category TEXT NOT NULL,"
        "  description TEXT,"
        "  price REAL NOT NULL,"
        "  discount_price REAL DEFAULT 0,"
        "  stock_quantity INTEGER NOT NULL DEFAULT 10,"
        "  cover_class TEXT,"
        "  is_featured INTEGER DEFAULT 0,"
        "  is_active INTEGER DEFAULT 1,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  google_id TEXT UNIQUE,"
        "  email TEXT UNIQUE NOT NULL,"
        "  name TEXT NOT NULL,"
        "  avatar_url TEXT,"
        "  role TEXT DEFAULT 'customer',"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  session_token TEXT PRIMARY KEY,"
        "  user_id INTEGER NOT NULL,"
        "  expires_at DATETIME NOT NULL,"
        "  FOREIGN KEY(user_id) REFERENCES users(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS orders ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  order_number TEXT UNIQUE NOT NULL,"
        "  user_id INTEGER NOT NULL,"
        "  customer_name TEXT NOT NULL,"
        "  phone TEXT NOT NULL,"
        "  delivery_address TEXT NOT NULL,"
        "  subtotal REAL NOT NULL,"
        "  delivery_charge REAL DEFAULT 60,"
        "  discount_amount REAL DEFAULT 0,"
        "  total_amount REAL NOT NULL,"
        "  status TEXT DEFAULT 'Pending',"
        "  payment_method TEXT DEFAULT 'Cash on Delivery',"
        "  payment_status TEXT DEFAULT 'Unpaid',"
        "  transaction_id TEXT,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(user_id) REFERENCES users(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS order_items ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  order_id INTEGER NOT NULL,"
        "  product_id INTEGER NOT NULL,"
        "  product_title TEXT NOT NULL,"
        "  unit_price REAL NOT NULL,"
        "  quantity INTEGER NOT NULL,"
        "  line_total REAL NOT NULL,"
        "  FOREIGN KEY(order_id) REFERENCES orders(id),"
        "  FOREIGN KEY(product_id) REFERENCES products(id)"
        ");";

    char *err_msg = NULL;
    rc = sqlite3_exec(g_db, schema_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Schema Error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    /* Seed initial demo products matching user's original catalog if empty */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM products", -1, &stmt, NULL);
    int product_count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        product_count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (product_count == 0) {
        const char *seed_sql = 
            "INSERT INTO products (sku, type, title, author, isbn, publisher, edition, language, category, description, price, stock_quantity, cover_class, is_featured) VALUES "
            "('B001', 'book', 'Monsoon Algorithms', 'N. Rahman', '978-984-01-001', 'Rain Press', '2nd Ed.', 'English', 'Computer Science', 'Sorting storms, graphs, and exam week. Essential for CSE214.', 890.0, 25, 'cover-book-1', 1),"
            "('B002', 'book', 'The Quiet Stack', 'A. Chowdhury', '978-984-01-002', 'Bengal Books', '1st Ed.', 'English', 'Fiction', 'A campus novel about libraries after hours.', 650.0, 18, 'cover-book-2', 1),"
            "('B003', 'book', 'Paper Weather', 'L. Ahmed', '978-984-01-003', 'Dhaka Literary', '1st Ed.', 'English', 'Essays', 'Short essays on reading in the rain.', 720.0, 12, 'cover-book-3', 1),"
            "('B004', 'book', 'Graph of Home', 'S. Islam', '978-984-01-004', 'Campus Academic', '3rd Ed.', 'English', 'Computer Science', 'Paths, trees, and finding your way back.', 980.0, 15, 'cover-book-4', 1);"
            "INSERT INTO products (sku, type, title, brand, color, size, specifications, category, description, price, stock_quantity, cover_class, is_featured) VALUES "
            "('S001', 'stationery', 'Rain Hall Notebook', 'Rain Stationery', 'Cream/Burgundy', 'A5', '120 pages, 100gsm dot grid', 'Notebooks', 'Dot grid paper for lectures and labs.', 220.0, 40, 'cover-stat-1', 1),"
            "('S002', 'stationery', 'Forest Ink Pen', 'Rain Stationery', 'Forest Green', '0.5mm', 'Smooth Japanese pigment ink', 'Pens & Ink', 'Smooth gel ink for long problem sets.', 340.0, 35, 'cover-stat-2', 1),"
            "('S003', 'stationery', 'Goldclip Highlighters', 'Pastel Desk', 'Pastel Yellow/Pink/Mint/Purple', 'Pack of 4', 'Chisel tip, water-based pastel ink', 'Desk Accessories', 'Soft tones that do not bleed through.', 180.0, 30, 'cover-stat-3', 1),"
            "('S004', 'stationery', 'Desk Storm Sticky Notes', 'Memo Rain', 'Muted Gray/Blue', '76x76mm', '3 pads x 50 sheets', 'Desk Accessories', 'Reminders for due dates and page marks.', 150.0, 50, 'cover-stat-4', 1);";

        sqlite3_exec(g_db, seed_sql, NULL, NULL, NULL);
    }

    /* Seed default Demo Users (Customer and Admin) */
    sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM users", -1, &stmt, NULL);
    int user_count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (user_count == 0) {
        const char *user_seed = 
            "INSERT INTO users (google_id, email, name, role) VALUES "
            "('demo-student-id', 'student@brishtilibrary.edu', 'CSE214 Student Demo', 'customer'),"
            "('demo-admin-id', 'admin@brishtilibrary.edu', 'Brishti Admin', 'admin');";
        sqlite3_exec(g_db, user_seed, NULL, NULL, NULL);
    }

    return SQLITE_OK;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

/* Products API */
char *db_get_products_json(const char *category, const char *search_q, const char *sort_by, const char *product_type) {
    char sql[1024];
    snprintf(sql, sizeof(sql), 
             "SELECT id, sku, type, title, author, brand, category, description, price, "
             "stock_quantity, cover_class, is_featured, is_active "
             "FROM products WHERE is_active = 1");

    if (product_type && strlen(product_type) > 0) {
        strncat(sql, " AND type = ?", sizeof(sql) - strlen(sql) - 1);
    }
    if (category && strlen(category) > 0 && strcmp(category, "all") != 0) {
        strncat(sql, " AND category = ?", sizeof(sql) - strlen(sql) - 1);
    }
    if (search_q && strlen(search_q) > 0) {
        strncat(sql, " AND (title LIKE ? OR author LIKE ? OR category LIKE ? OR sku LIKE ?)", sizeof(sql) - strlen(sql) - 1);
    }

    if (sort_by && strcmp(sort_by, "price_asc") == 0) {
        strncat(sql, " ORDER BY price ASC", sizeof(sql) - strlen(sql) - 1);
    } else if (sort_by && strcmp(sort_by, "price_desc") == 0) {
        strncat(sql, " ORDER BY price DESC", sizeof(sql) - strlen(sql) - 1);
    } else if (sort_by && strcmp(sort_by, "name_asc") == 0) {
        strncat(sql, " ORDER BY title ASC", sizeof(sql) - strlen(sql) - 1);
    } else {
        strncat(sql, " ORDER BY is_featured DESC, id ASC", sizeof(sql) - strlen(sql) - 1);
    }

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "error", sqlite3_errmsg(g_db));
        char *s = cJSON_PrintUnformatted(err);
        cJSON_Delete(err);
        return s;
    }

    int bind_idx = 1;
    if (product_type && strlen(product_type) > 0) {
        sqlite3_bind_text(stmt, bind_idx++, product_type, -1, SQLITE_STATIC);
    }
    if (category && strlen(category) > 0 && strcmp(category, "all") != 0) {
        sqlite3_bind_text(stmt, bind_idx++, category, -1, SQLITE_STATIC);
    }
    char like_pattern[128] = "";
    if (search_q && strlen(search_q) > 0) {
        snprintf(like_pattern, sizeof(like_pattern), "%%%s%%", search_q);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
    }

    cJSON *arr = cJSON_CreateArray();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *p = cJSON_CreateObject();
        int id = sqlite3_column_int(stmt, 0);
        cJSON_AddNumberToObject(p, "id", id);
        cJSON_AddStringToObject(p, "sku", (const char*)sqlite3_column_text(stmt, 1));
        const char *type = (const char*)sqlite3_column_text(stmt, 2);
        cJSON_AddStringToObject(p, "type", type);
        cJSON_AddStringToObject(p, "title", (const char*)sqlite3_column_text(stmt, 3));
        
        const char *author = (const char*)sqlite3_column_text(stmt, 4);
        const char *brand = (const char*)sqlite3_column_text(stmt, 5);
        if (author) cJSON_AddStringToObject(p, "author", author);
        if (brand) cJSON_AddStringToObject(p, "brand", brand);

        cJSON_AddStringToObject(p, "category", (const char*)sqlite3_column_text(stmt, 6));
        cJSON_AddStringToObject(p, "description", (const char*)sqlite3_column_text(stmt, 7));
        cJSON_AddNumberToObject(p, "price", sqlite3_column_double(stmt, 8));
        cJSON_AddNumberToObject(p, "stock_quantity", sqlite3_column_int(stmt, 9));
        
        const char *cov = (const char*)sqlite3_column_text(stmt, 10);
        cJSON_AddStringToObject(p, "cover_class", cov ? cov : "cover-book-1");
        cJSON_AddBoolToObject(p, "is_featured", sqlite3_column_int(stmt, 11));

        /* Format display meta string matching frontend card expectations */
        char meta_buf[128];
        if (strcmp(type, "book") == 0 && author) {
            snprintf(meta_buf, sizeof(meta_buf), "%s · %s", author, (const char*)sqlite3_column_text(stmt, 6));
        } else if (brand) {
            snprintf(meta_buf, sizeof(meta_buf), "%s", brand);
        } else {
            snprintf(meta_buf, sizeof(meta_buf), "%s", (const char*)sqlite3_column_text(stmt, 6));
        }
        cJSON_AddStringToObject(p, "meta", meta_buf);

        cJSON_AddItemToArray(arr, p);
    }
    sqlite3_finalize(stmt);

    char *out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}

char *db_get_product_by_id_json(int id) {
    const char *sql = "SELECT id, sku, type, title, author, isbn, publisher, edition, language, "
                      "brand, color, size, specifications, category, description, price, "
                      "stock_quantity, cover_class, is_featured, is_active FROM products WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(p, "sku", (const char*)sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(p, "type", (const char*)sqlite3_column_text(stmt, 2));
        cJSON_AddStringToObject(p, "title", (const char*)sqlite3_column_text(stmt, 3));
        
        const char *author = (const char*)sqlite3_column_text(stmt, 4);
        if (author) cJSON_AddStringToObject(p, "author", author);
        const char *isbn = (const char*)sqlite3_column_text(stmt, 5);
        if (isbn) cJSON_AddStringToObject(p, "isbn", isbn);
        const char *publisher = (const char*)sqlite3_column_text(stmt, 6);
        if (publisher) cJSON_AddStringToObject(p, "publisher", publisher);
        const char *edition = (const char*)sqlite3_column_text(stmt, 7);
        if (edition) cJSON_AddStringToObject(p, "edition", edition);
        const char *lang = (const char*)sqlite3_column_text(stmt, 8);
        if (lang) cJSON_AddStringToObject(p, "language", lang);

        const char *brand = (const char*)sqlite3_column_text(stmt, 9);
        if (brand) cJSON_AddStringToObject(p, "brand", brand);
        const char *color = (const char*)sqlite3_column_text(stmt, 10);
        if (color) cJSON_AddStringToObject(p, "color", color);
        const char *size = (const char*)sqlite3_column_text(stmt, 11);
        if (size) cJSON_AddStringToObject(p, "size", size);
        const char *specs = (const char*)sqlite3_column_text(stmt, 12);
        if (specs) cJSON_AddStringToObject(p, "specifications", specs);

        cJSON_AddStringToObject(p, "category", (const char*)sqlite3_column_text(stmt, 13));
        cJSON_AddStringToObject(p, "description", (const char*)sqlite3_column_text(stmt, 14));
        cJSON_AddNumberToObject(p, "price", sqlite3_column_double(stmt, 15));
        cJSON_AddNumberToObject(p, "stock_quantity", sqlite3_column_int(stmt, 16));
        cJSON_AddStringToObject(p, "cover_class", (const char*)sqlite3_column_text(stmt, 17));
        cJSON_AddBoolToObject(p, "is_featured", sqlite3_column_int(stmt, 18));
        cJSON_AddBoolToObject(p, "is_active", sqlite3_column_int(stmt, 19));

        sqlite3_finalize(stmt);
        char *out = cJSON_PrintUnformatted(p);
        cJSON_Delete(p);
        return out;
    }
    sqlite3_finalize(stmt);
    return NULL;
}

char *db_get_categories_json(void) {
    const char *sql = "SELECT DISTINCT category, type FROM products WHERE is_active = 1 ORDER BY category ASC";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    cJSON *arr = cJSON_CreateArray();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *cat = cJSON_CreateObject();
        cJSON_AddStringToObject(cat, "name", (const char*)sqlite3_column_text(stmt, 0));
        cJSON_AddStringToObject(cat, "type", (const char*)sqlite3_column_text(stmt, 1));
        cJSON_AddItemToArray(arr, cat);
    }
    sqlite3_finalize(stmt);

    char *out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}

/* Cart Validation & Price Verification (Server-Side) */
char *db_validate_cart_json(const char *cart_json_str) {
    cJSON *cart = cJSON_Parse(cart_json_str);
    cJSON *resp = cJSON_CreateObject();

    if (!cart || !cJSON_IsArray(cart)) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid cart payload");
        if (cart) cJSON_Delete(cart);
        char *err_s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return err_s;
    }

    double subtotal = 0.0;
    double delivery_charge = 60.0;
    bool all_in_stock = true;
    cJSON *verified_items = cJSON_CreateArray();

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, cart) {
        cJSON *id_obj = cJSON_GetObjectItem(item, "id");
        cJSON *qty_obj = cJSON_GetObjectItem(item, "qty");
        if (!id_obj || !qty_obj) continue;

        int req_qty = qty_obj->valueint;
        if (req_qty <= 0) continue;

        sqlite3_stmt *stmt;
        const char *sql = "SELECT id, sku, title, price, stock_quantity FROM products WHERE id = ? OR sku = ?";
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) continue;

        if (cJSON_IsNumber(id_obj)) {
            sqlite3_bind_int(stmt, 1, id_obj->valueint);
            sqlite3_bind_text(stmt, 2, "", -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_int(stmt, 1, -1);
            sqlite3_bind_text(stmt, 2, id_obj->valuestring, -1, SQLITE_STATIC);
        }

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int prod_id = sqlite3_column_int(stmt, 0);
            const char *title = (const char*)sqlite3_column_text(stmt, 2);
            double unit_price = sqlite3_column_double(stmt, 3);
            int available_stock = sqlite3_column_int(stmt, 4);

            cJSON *v_item = cJSON_CreateObject();
            cJSON_AddNumberToObject(v_item, "id", prod_id);
            cJSON_AddStringToObject(v_item, "title", title);
            cJSON_AddNumberToObject(v_item, "unit_price", unit_price);
            cJSON_AddNumberToObject(v_item, "requested_qty", req_qty);
            cJSON_AddNumberToObject(v_item, "available_stock", available_stock);

            int allowed_qty = (req_qty <= available_stock) ? req_qty : available_stock;
            if (req_qty > available_stock) {
                all_in_stock = false;
                cJSON_AddBoolToObject(v_item, "stock_warning", true);
            } else {
                cJSON_AddBoolToObject(v_item, "stock_warning", false);
            }
            cJSON_AddNumberToObject(v_item, "effective_qty", allowed_qty);

            double line_total = unit_price * allowed_qty;
            cJSON_AddNumberToObject(v_item, "line_total", line_total);
            subtotal += line_total;

            cJSON_AddItemToArray(verified_items, v_item);
        }
        sqlite3_finalize(stmt);
    }
    cJSON_Delete(cart);

    if (subtotal == 0.0) delivery_charge = 0.0;
    double total = subtotal + delivery_charge;

    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddBoolToObject(resp, "all_in_stock", all_in_stock);
    cJSON_AddNumberToObject(resp, "subtotal", subtotal);
    cJSON_AddNumberToObject(resp, "delivery_charge", delivery_charge);
    cJSON_AddNumberToObject(resp, "total", total);
    cJSON_AddItemToObject(resp, "items", verified_items);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* Order Creation with Transactional Safety & Stock Decrement */
char *db_create_order_json(int user_id, const char *order_json_str) {
    cJSON *order_data = cJSON_Parse(order_json_str);
    cJSON *resp = cJSON_CreateObject();

    if (!order_data) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid order payload");
        char *err_s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return err_s;
    }

    cJSON *name_obj = cJSON_GetObjectItem(order_data, "customer_name");
    cJSON *phone_obj = cJSON_GetObjectItem(order_data, "phone");
    cJSON *addr_obj = cJSON_GetObjectItem(order_data, "delivery_address");
    cJSON *items_obj = cJSON_GetObjectItem(order_data, "items");
    cJSON *payment_method_obj = cJSON_GetObjectItem(order_data, "payment_method");

    if (!name_obj || !phone_obj || !addr_obj || !items_obj || !cJSON_IsArray(items_obj)) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Missing required fields: name, phone, address, or items");
        cJSON_Delete(order_data);
        char *err_s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return err_s;
    }

    /* Start SQLite Transaction */
    sqlite3_exec(g_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    double subtotal = 0.0;
    double delivery_charge = 60.0;

    /* First pass: verify stock availability for all items */
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, items_obj) {
        cJSON *id_obj = cJSON_GetObjectItem(item, "id");
        cJSON *qty_obj = cJSON_GetObjectItem(item, "qty");
        if (!id_obj || !qty_obj) continue;

        int req_qty = qty_obj->valueint;
        sqlite3_stmt *stmt;
        const char *sql = "SELECT price, stock_quantity, title FROM products WHERE id = ? OR sku = ?";
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
            sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
            cJSON_AddStringToObject(resp, "status", "error");
            cJSON_AddStringToObject(resp, "message", "Database error checking items");
            cJSON_Delete(order_data);
            char *s = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return s;
        }

        if (cJSON_IsNumber(id_obj)) {
            sqlite3_bind_int(stmt, 1, id_obj->valueint);
            sqlite3_bind_text(stmt, 2, "", -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_int(stmt, 1, -1);
            sqlite3_bind_text(stmt, 2, id_obj->valuestring, -1, SQLITE_STATIC);
        }

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double price = sqlite3_column_double(stmt, 0);
            int stock = sqlite3_column_int(stmt, 1);
            const char *title = (const char*)sqlite3_column_text(stmt, 2);

            if (req_qty > stock) {
                char err_buf[256];
                snprintf(err_buf, sizeof(err_buf), "Insufficient stock for item: %s. Available: %d, Requested: %d",
                         title, stock, req_qty);
                sqlite3_finalize(stmt);
                sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);

                cJSON_AddStringToObject(resp, "status", "error");
                cJSON_AddStringToObject(resp, "message", err_buf);
                cJSON_Delete(order_data);
                char *s = cJSON_PrintUnformatted(resp);
                cJSON_Delete(resp);
                return s;
            }
            subtotal += price * req_qty;
        }
        sqlite3_finalize(stmt);
    }

    if (subtotal == 0.0) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Cart contains no valid items");
        cJSON_Delete(order_data);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    double total_amount = subtotal + delivery_charge;

    /* Generate unique order number */
    char order_number[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(order_number, sizeof(order_number), "BL-%04d%02d%02d-%04d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, (rand() % 9000) + 1000);

    const char *pay_method = (payment_method_obj && payment_method_obj->valuestring) 
                             ? payment_method_obj->valuestring : "Cash on Delivery";

    /* Insert into orders table */
    const char *ins_order_sql = 
        "INSERT INTO orders (order_number, user_id, customer_name, phone, delivery_address, "
        "subtotal, delivery_charge, total_amount, status, payment_method, payment_status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'Pending', ?, 'Unpaid')";

    sqlite3_stmt *order_stmt;
    if (sqlite3_prepare_v2(g_db, ins_order_sql, -1, &order_stmt, NULL) != SQLITE_OK) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Failed to create order record");
        cJSON_Delete(order_data);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    sqlite3_bind_text(order_stmt, 1, order_number, -1, SQLITE_STATIC);
    sqlite3_bind_int(order_stmt, 2, user_id);
    sqlite3_bind_text(order_stmt, 3, name_obj->valuestring, -1, SQLITE_STATIC);
    sqlite3_bind_text(order_stmt, 4, phone_obj->valuestring, -1, SQLITE_STATIC);
    sqlite3_bind_text(order_stmt, 5, addr_obj->valuestring, -1, SQLITE_STATIC);
    sqlite3_bind_double(order_stmt, 6, subtotal);
    sqlite3_bind_double(order_stmt, 7, delivery_charge);
    sqlite3_bind_double(order_stmt, 8, total_amount);
    sqlite3_bind_text(order_stmt, 9, pay_method, -1, SQLITE_STATIC);

    if (sqlite3_step(order_stmt) != SQLITE_DONE) {
        sqlite3_finalize(order_stmt);
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Failed to insert order");
        cJSON_Delete(order_data);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }
    sqlite3_finalize(order_stmt);
    sqlite3_int64 order_id = sqlite3_last_insert_rowid(g_db);

    /* Second pass: insert order_items and decrement stock */
    cJSON_ArrayForEach(item, items_obj) {
        cJSON *id_obj = cJSON_GetObjectItem(item, "id");
        cJSON *qty_obj = cJSON_GetObjectItem(item, "qty");
        if (!id_obj || !qty_obj) continue;
        int req_qty = qty_obj->valueint;

        sqlite3_stmt *sel_stmt;
        const char *sel_sql = "SELECT id, title, price FROM products WHERE id = ? OR sku = ?";
        sqlite3_prepare_v2(g_db, sel_sql, -1, &sel_stmt, NULL);
        if (cJSON_IsNumber(id_obj)) {
            sqlite3_bind_int(sel_stmt, 1, id_obj->valueint);
            sqlite3_bind_text(sel_stmt, 2, "", -1, SQLITE_STATIC);
        } else {
            sqlite3_bind_int(sel_stmt, 1, -1);
            sqlite3_bind_text(sel_stmt, 2, id_obj->valuestring, -1, SQLITE_STATIC);
        }

        if (sqlite3_step(sel_stmt) == SQLITE_ROW) {
            int prod_id = sqlite3_column_int(sel_stmt, 0);
            const char *title = (const char*)sqlite3_column_text(sel_stmt, 1);
            double price = sqlite3_column_double(sel_stmt, 2);
            double line_total = price * req_qty;

            /* Insert order_item */
            sqlite3_stmt *item_stmt;
            const char *ins_item_sql = "INSERT INTO order_items (order_id, product_id, product_title, unit_price, quantity, line_total) VALUES (?, ?, ?, ?, ?, ?)";
            sqlite3_prepare_v2(g_db, ins_item_sql, -1, &item_stmt, NULL);
            sqlite3_bind_int64(item_stmt, 1, order_id);
            sqlite3_bind_int(item_stmt, 2, prod_id);
            sqlite3_bind_text(item_stmt, 3, title, -1, SQLITE_STATIC);
            sqlite3_bind_double(item_stmt, 4, price);
            sqlite3_bind_int(item_stmt, 5, req_qty);
            sqlite3_bind_double(item_stmt, 6, line_total);
            sqlite3_step(item_stmt);
            sqlite3_finalize(item_stmt);

            /* Decrement stock */
            sqlite3_stmt *stock_stmt;
            const char *dec_sql = "UPDATE products SET stock_quantity = stock_quantity - ? WHERE id = ?";
            sqlite3_prepare_v2(g_db, dec_sql, -1, &stock_stmt, NULL);
            sqlite3_bind_int(stock_stmt, 1, req_qty);
            sqlite3_bind_int(stock_stmt, 2, prod_id);
            sqlite3_step(stock_stmt);
            sqlite3_finalize(stock_stmt);
        }
        sqlite3_finalize(sel_stmt);
    }

    /* Commit Transaction */
    sqlite3_exec(g_db, "COMMIT;", NULL, NULL, NULL);

    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Order placed successfully");
    cJSON_AddStringToObject(resp, "order_number", order_number);
    cJSON_AddNumberToObject(resp, "order_id", (double)order_id);
    cJSON_AddNumberToObject(resp, "subtotal", subtotal);
    cJSON_AddNumberToObject(resp, "delivery_charge", delivery_charge);
    cJSON_AddNumberToObject(resp, "total_amount", total_amount);
    cJSON_AddStringToObject(resp, "order_status", "Pending");

    cJSON_Delete(order_data);
    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* Retrieve User Orders */
char *db_get_user_orders_json(int user_id) {
    const char *sql = "SELECT id, order_number, customer_name, phone, delivery_address, "
                      "subtotal, delivery_charge, total_amount, status, payment_method, "
                      "payment_status, created_at FROM orders WHERE user_id = ? ORDER BY id DESC";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    sqlite3_bind_int(stmt, 1, user_id);

    cJSON *arr = cJSON_CreateArray();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *ord = cJSON_CreateObject();
        int ord_id = sqlite3_column_int(stmt, 0);
        cJSON_AddNumberToObject(ord, "id", ord_id);
        cJSON_AddStringToObject(ord, "order_number", (const char*)sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(ord, "customer_name", (const char*)sqlite3_column_text(stmt, 2));
        cJSON_AddStringToObject(ord, "phone", (const char*)sqlite3_column_text(stmt, 3));
        cJSON_AddStringToObject(ord, "delivery_address", (const char*)sqlite3_column_text(stmt, 4));
        cJSON_AddNumberToObject(ord, "subtotal", sqlite3_column_double(stmt, 5));
        cJSON_AddNumberToObject(ord, "delivery_charge", sqlite3_column_double(stmt, 6));
        cJSON_AddNumberToObject(ord, "total_amount", sqlite3_column_double(stmt, 7));
        cJSON_AddStringToObject(ord, "status", (const char*)sqlite3_column_text(stmt, 8));
        cJSON_AddStringToObject(ord, "payment_method", (const char*)sqlite3_column_text(stmt, 9));
        cJSON_AddStringToObject(ord, "payment_status", (const char*)sqlite3_column_text(stmt, 10));
        cJSON_AddStringToObject(ord, "created_at", (const char*)sqlite3_column_text(stmt, 11));

        /* Fetch order items */
        sqlite3_stmt *item_stmt;
        const char *item_sql = "SELECT product_title, unit_price, quantity, line_total FROM order_items WHERE order_id = ?";
        sqlite3_prepare_v2(g_db, item_sql, -1, &item_stmt, NULL);
        sqlite3_bind_int(item_stmt, 1, ord_id);
        cJSON *items_arr = cJSON_CreateArray();
        while (sqlite3_step(item_stmt) == SQLITE_ROW) {
            cJSON *oi = cJSON_CreateObject();
            cJSON_AddStringToObject(oi, "title", (const char*)sqlite3_column_text(item_stmt, 0));
            cJSON_AddNumberToObject(oi, "unit_price", sqlite3_column_double(item_stmt, 1));
            cJSON_AddNumberToObject(oi, "quantity", sqlite3_column_int(item_stmt, 2));
            cJSON_AddNumberToObject(oi, "line_total", sqlite3_column_double(item_stmt, 3));
            cJSON_AddItemToArray(items_arr, oi);
        }
        sqlite3_finalize(item_stmt);
        cJSON_AddItemToObject(ord, "items", items_arr);

        cJSON_AddItemToArray(arr, ord);
    }
    sqlite3_finalize(stmt);

    char *out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}

/* User & Session Management */
int db_find_or_create_user(const char *email, const char *name, const char *google_id, const char *role) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, "SELECT id, role FROM users WHERE email = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int uid = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return uid;
    }
    sqlite3_finalize(stmt);

    /* Insert new user */
    const char *ins_sql = "INSERT INTO users (email, name, google_id, role) VALUES (?, ?, ?, ?)";
    sqlite3_prepare_v2(g_db, ins_sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, google_id ? google_id : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, role ? role : "customer", -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (int)sqlite3_last_insert_rowid(g_db);
}

char *db_create_session(int user_id) {
    char token[65];
    generate_random_token(token, sizeof(token));

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO sessions (session_token, user_id, expires_at) "
                      "VALUES (?, ?, datetime('now', '+7 days'))";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    char *res = (char *)malloc(strlen(token) + 1);
    if (res) strcpy(res, token);
    return res;
}

int db_validate_session(const char *session_token, char *out_name, char *out_email, char *out_role) {
    if (!session_token || strlen(session_token) == 0) return 0;

    sqlite3_stmt *stmt;
    const char *sql = "SELECT u.id, u.name, u.email, u.role FROM sessions s "
                      "JOIN users u ON s.user_id = u.id "
                      "WHERE s.session_token = ? AND s.expires_at > datetime('now')";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, session_token, -1, SQLITE_STATIC);

    int uid = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        uid = sqlite3_column_int(stmt, 0);
        if (out_name) strcpy(out_name, (const char*)sqlite3_column_text(stmt, 1));
        if (out_email) strcpy(out_email, (const char*)sqlite3_column_text(stmt, 2));
        if (out_role) strcpy(out_role, (const char*)sqlite3_column_text(stmt, 3));
    }
    sqlite3_finalize(stmt);
    return uid;
}

void db_delete_session(const char *session_token) {
    if (!session_token) return;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, "DELETE FROM sessions WHERE session_token = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, session_token, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

/* Admin Orders & Products */
char *db_admin_get_all_orders_json(void) {
    const char *sql = "SELECT id, order_number, customer_name, phone, delivery_address, "
                      "subtotal, delivery_charge, total_amount, status, payment_method, "
                      "payment_status, created_at FROM orders ORDER BY id DESC";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;

    cJSON *arr = cJSON_CreateArray();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *ord = cJSON_CreateObject();
        int ord_id = sqlite3_column_int(stmt, 0);
        cJSON_AddNumberToObject(ord, "id", ord_id);
        cJSON_AddStringToObject(ord, "order_number", (const char*)sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(ord, "customer_name", (const char*)sqlite3_column_text(stmt, 2));
        cJSON_AddStringToObject(ord, "phone", (const char*)sqlite3_column_text(stmt, 3));
        cJSON_AddStringToObject(ord, "delivery_address", (const char*)sqlite3_column_text(stmt, 4));
        cJSON_AddNumberToObject(ord, "subtotal", sqlite3_column_double(stmt, 5));
        cJSON_AddNumberToObject(ord, "delivery_charge", sqlite3_column_double(stmt, 6));
        cJSON_AddNumberToObject(ord, "total_amount", sqlite3_column_double(stmt, 7));
        cJSON_AddStringToObject(ord, "status", (const char*)sqlite3_column_text(stmt, 8));
        cJSON_AddStringToObject(ord, "payment_method", (const char*)sqlite3_column_text(stmt, 9));
        cJSON_AddStringToObject(ord, "payment_status", (const char*)sqlite3_column_text(stmt, 10));
        cJSON_AddStringToObject(ord, "created_at", (const char*)sqlite3_column_text(stmt, 11));
        cJSON_AddItemToArray(arr, ord);
    }
    sqlite3_finalize(stmt);

    char *out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}

char *db_admin_update_order_status_json(int order_id, const char *new_status) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, "UPDATE orders SET status = ? WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, new_status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, order_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    cJSON *resp = cJSON_CreateObject();
    if (rc == SQLITE_DONE) {
        cJSON_AddStringToObject(resp, "status", "success");
        cJSON_AddStringToObject(resp, "message", "Order status updated");
    } else {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Failed to update order status");
    }
    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}
