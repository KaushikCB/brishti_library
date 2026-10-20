#include "../include/database.h"
#include "../include/cJSON.h"
#include "../include/sha256.h"
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
        "  image_url TEXT,"
        "  is_featured INTEGER DEFAULT 0,"
        "  is_active INTEGER DEFAULT 1,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS categories ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT UNIQUE NOT NULL,"
        "  type TEXT NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  google_id TEXT UNIQUE,"
        "  email TEXT UNIQUE NOT NULL,"
        "  name TEXT NOT NULL,"
        "  avatar_url TEXT,"
        "  role TEXT DEFAULT 'customer',"
        "  password_hash TEXT,"
        "  password_salt TEXT,"
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
        ");"
        "CREATE INDEX IF NOT EXISTS idx_products_type_cat ON products(type, category);"
        "CREATE INDEX IF NOT EXISTS idx_products_active ON products(is_active);"
        "CREATE INDEX IF NOT EXISTS idx_orders_user ON orders(user_id);"
        "CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token);"
        "CREATE INDEX IF NOT EXISTS idx_order_items_order ON order_items(order_id);";

    char *err_msg = NULL;
    rc = sqlite3_exec(g_db, schema_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL Schema Error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    /* Schema migrations for existing databases */
    sqlite3_exec(g_db, "ALTER TABLE users ADD COLUMN password_hash TEXT;", NULL, NULL, NULL);
    sqlite3_exec(g_db, "ALTER TABLE users ADD COLUMN password_salt TEXT;", NULL, NULL, NULL);
    sqlite3_exec(g_db, "ALTER TABLE products ADD COLUMN image_url TEXT;", NULL, NULL, NULL);

    /* Seed categories if empty */
    sqlite3_stmt *cat_stmt;
    sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM categories", -1, &cat_stmt, NULL);
    int cat_count = 0;
    if (sqlite3_step(cat_stmt) == SQLITE_ROW) {
        cat_count = sqlite3_column_int(cat_stmt, 0);
    }
    sqlite3_finalize(cat_stmt);

    if (cat_count == 0) {
        const char *cat_seed = 
            "INSERT INTO categories (name, type) VALUES "
            "('Computer Science', 'book'),"
            "('Fiction', 'book'),"
            "('Essays', 'book'),"
            "('Notebooks', 'stationery'),"
            "('Pens & Ink', 'stationery'),"
            "('Desk Accessories', 'stationery');";
        sqlite3_exec(g_db, cat_seed, NULL, NULL, NULL);
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
char *db_get_products_json(const char *category, const char *search_q, const char *sort_by, 
                           const char *product_type, const char *availability, 
                           double min_price, double max_price, int limit, int offset) {
    char sql[1536];
    snprintf(sql, sizeof(sql), 
             "SELECT id, sku, type, title, author, brand, category, description, price, "
             "stock_quantity, cover_class, is_featured, is_active, "
             "isbn, publisher, edition, language, color, size, specifications, discount_price, image_url "
             "FROM products WHERE is_active = 1");

    if (product_type && strlen(product_type) > 0) {
        strncat(sql, " AND type = ?", sizeof(sql) - strlen(sql) - 1);
    }
    if (category && strlen(category) > 0 && strcmp(category, "all") != 0) {
        strncat(sql, " AND category = ?", sizeof(sql) - strlen(sql) - 1);
    }
    if (availability && strcmp(availability, "in_stock") == 0) {
        strncat(sql, " AND stock_quantity > 0", sizeof(sql) - strlen(sql) - 1);
    }
    if (min_price > 0.0) {
        strncat(sql, " AND price >= ?", sizeof(sql) - strlen(sql) - 1);
    }
    if (max_price > 0.0 && max_price >= min_price) {
        strncat(sql, " AND price <= ?", sizeof(sql) - strlen(sql) - 1);
    }
    if (search_q && strlen(search_q) > 0) {
        strncat(sql, " AND (title LIKE ? OR author LIKE ? OR category LIKE ? OR sku LIKE ? OR brand LIKE ? OR isbn LIKE ?)", sizeof(sql) - strlen(sql) - 1);
    }

    if (sort_by && strcmp(sort_by, "price_asc") == 0) {
        strncat(sql, " ORDER BY price ASC", sizeof(sql) - strlen(sql) - 1);
    } else if (sort_by && strcmp(sort_by, "price_desc") == 0) {
        strncat(sql, " ORDER BY price DESC", sizeof(sql) - strlen(sql) - 1);
    } else if (sort_by && strcmp(sort_by, "name_asc") == 0) {
        strncat(sql, " ORDER BY title ASC", sizeof(sql) - strlen(sql) - 1);
    } else if (sort_by && strcmp(sort_by, "name_desc") == 0) {
        strncat(sql, " ORDER BY title DESC", sizeof(sql) - strlen(sql) - 1);
    } else {
        strncat(sql, " ORDER BY is_featured DESC, id ASC", sizeof(sql) - strlen(sql) - 1);
    }

    if (limit > 0) {
        strncat(sql, " LIMIT ? OFFSET ?", sizeof(sql) - strlen(sql) - 1);
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
    if (min_price > 0.0) {
        sqlite3_bind_double(stmt, bind_idx++, min_price);
    }
    if (max_price > 0.0 && max_price >= min_price) {
        sqlite3_bind_double(stmt, bind_idx++, max_price);
    }
    char like_pattern[128] = "";
    if (search_q && strlen(search_q) > 0) {
        snprintf(like_pattern, sizeof(like_pattern), "%%%s%%", search_q);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, bind_idx++, like_pattern, -1, SQLITE_STATIC);
    }
    if (limit > 0) {
        sqlite3_bind_int(stmt, bind_idx++, limit);
        sqlite3_bind_int(stmt, bind_idx++, offset > 0 ? offset : 0);
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
        cJSON_AddBoolToObject(p, "is_active", sqlite3_column_int(stmt, 12));

        const char *isbn = (const char*)sqlite3_column_text(stmt, 13);
        const char *pub = (const char*)sqlite3_column_text(stmt, 14);
        const char *ed = (const char*)sqlite3_column_text(stmt, 15);
        const char *lang = (const char*)sqlite3_column_text(stmt, 16);
        const char *color = (const char*)sqlite3_column_text(stmt, 17);
        const char *size = (const char*)sqlite3_column_text(stmt, 18);
        const char *specs = (const char*)sqlite3_column_text(stmt, 19);
        double disc_price = sqlite3_column_double(stmt, 20);

        if (isbn && strlen(isbn) > 0) cJSON_AddStringToObject(p, "isbn", isbn);
        if (pub && strlen(pub) > 0) cJSON_AddStringToObject(p, "publisher", pub);
        if (ed && strlen(ed) > 0) cJSON_AddStringToObject(p, "edition", ed);
        if (lang && strlen(lang) > 0) cJSON_AddStringToObject(p, "language", lang);
        if (color && strlen(color) > 0) cJSON_AddStringToObject(p, "color", color);
        if (size && strlen(size) > 0) cJSON_AddStringToObject(p, "size", size);
        if (specs && strlen(specs) > 0) cJSON_AddStringToObject(p, "specifications", specs);
        if (disc_price > 0.0) cJSON_AddNumberToObject(p, "discount_price", disc_price);

        const char *img_url = (const char*)sqlite3_column_text(stmt, 21);
        if (img_url && strlen(img_url) > 0) cJSON_AddStringToObject(p, "image_url", img_url);
        else cJSON_AddStringToObject(p, "image_url", "");

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
                      "stock_quantity, cover_class, is_featured, is_active, image_url FROM products WHERE id = ?";
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

        const char *img_url = (const char*)sqlite3_column_text(stmt, 20);
        if (img_url && strlen(img_url) > 0) cJSON_AddStringToObject(p, "image_url", img_url);
        else cJSON_AddStringToObject(p, "image_url", "");

        sqlite3_finalize(stmt);
        char *out = cJSON_PrintUnformatted(p);
        cJSON_Delete(p);
        return out;
    }
    sqlite3_finalize(stmt);
    return NULL;
}

char *db_get_categories_json(void) {
    const char *sql = 
        "SELECT c.id, c.name, c.type, COUNT(p.id) AS product_count "
        "FROM categories c "
        "LEFT JOIN products p ON p.category = c.name AND p.is_active = 1 "
        "GROUP BY c.id, c.name, c.type "
        "ORDER BY c.type ASC, c.name ASC";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    cJSON *arr = cJSON_CreateArray();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *cat = cJSON_CreateObject();
        cJSON_AddNumberToObject(cat, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(cat, "name", (const char*)sqlite3_column_text(stmt, 1));
        cJSON_AddStringToObject(cat, "type", (const char*)sqlite3_column_text(stmt, 2));
        cJSON_AddNumberToObject(cat, "product_count", sqlite3_column_int(stmt, 3));
        cJSON_AddItemToArray(arr, cat);
    }
    sqlite3_finalize(stmt);

    char *out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}

char *db_admin_create_category_json(const char *json_body) {
    cJSON *resp = cJSON_CreateObject();
    cJSON *root = cJSON_Parse(json_body);
    if (!root) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid JSON payload");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    cJSON *name_obj = cJSON_GetObjectItem(root, "name");
    cJSON *type_obj = cJSON_GetObjectItem(root, "type");
    if (!name_obj || !name_obj->valuestring || strlen(name_obj->valuestring) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Category name is required");
        cJSON_Delete(root);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    const char *cat_name = name_obj->valuestring;
    const char *cat_type = (type_obj && type_obj->valuestring && strcmp(type_obj->valuestring, "stationery") == 0) 
                           ? "stationery" : "book";

    /* Check if already exists */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, "SELECT id FROM categories WHERE lower(name) = lower(?)", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, cat_name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Category with this name already exists");
        cJSON_Delete(root);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }
    sqlite3_finalize(stmt);

    const char *ins_sql = "INSERT INTO categories (name, type) VALUES (?, ?)";
    sqlite3_prepare_v2(g_db, ins_sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, cat_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, cat_type, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Failed to insert category into database");
        cJSON_Delete(root);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }
    sqlite3_finalize(stmt);
    sqlite3_int64 new_id = sqlite3_last_insert_rowid(g_db);

    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Category created successfully");
    cJSON_AddNumberToObject(resp, "id", (double)new_id);
    cJSON_AddStringToObject(resp, "name", cat_name);
    cJSON_AddStringToObject(resp, "type", cat_type);

    cJSON_Delete(root);
    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

char *db_admin_update_category_json(int id, const char *json_body) {
    cJSON *resp = cJSON_CreateObject();
    if (id <= 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid category ID");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    cJSON *root = cJSON_Parse(json_body);
    if (!root) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid JSON payload");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    cJSON *name_obj = cJSON_GetObjectItem(root, "name");
    cJSON *type_obj = cJSON_GetObjectItem(root, "type");
    if (!name_obj || !name_obj->valuestring || strlen(name_obj->valuestring) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Category name is required");
        cJSON_Delete(root);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    /* Retrieve old name */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, "SELECT name, type FROM categories WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    char old_name[128] = "";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        strncpy(old_name, (const char*)sqlite3_column_text(stmt, 0), sizeof(old_name) - 1);
    }
    sqlite3_finalize(stmt);

    if (strlen(old_name) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Category not found");
        cJSON_Delete(root);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    const char *new_name = name_obj->valuestring;
    const char *new_type = (type_obj && type_obj->valuestring && strcmp(type_obj->valuestring, "stationery") == 0) ? "stationery" : "book";

    /* Update category */
    sqlite3_prepare_v2(g_db, "UPDATE categories SET name = ?, type = ? WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, new_type, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    /* Cascade update product category names if renamed */
    if (strcmp(old_name, new_name) != 0) {
        sqlite3_prepare_v2(g_db, "UPDATE products SET category = ? WHERE category = ?", -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, new_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, old_name, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Category updated successfully");
    cJSON_AddNumberToObject(resp, "id", id);
    cJSON_AddStringToObject(resp, "name", new_name);
    cJSON_AddStringToObject(resp, "type", new_type);

    cJSON_Delete(root);
    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

char *db_admin_delete_category_json(int id) {
    cJSON *resp = cJSON_CreateObject();
    if (id <= 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid category ID");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    /* Find category name */
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, "SELECT name FROM categories WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    char cat_name[128] = "";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        strncpy(cat_name, (const char*)sqlite3_column_text(stmt, 0), sizeof(cat_name) - 1);
    }
    sqlite3_finalize(stmt);

    if (strlen(cat_name) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Category not found");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    /* Prevent unsafe deletion of categories containing products */
    sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM products WHERE category = ? AND is_active = 1", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, cat_name, -1, SQLITE_STATIC);
    int prod_count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        prod_count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (prod_count > 0) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Cannot delete category containing %d active product(s). Please reassign or delete the products first.", prod_count);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", err_msg);
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    /* Safe to delete */
    sqlite3_prepare_v2(g_db, "DELETE FROM categories WHERE id = ?", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Category deleted successfully");
    cJSON_AddNumberToObject(resp, "id", id);

    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
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

char *db_admin_get_all_products_json(void) {
    const char *sql = 
        "SELECT id, sku, type, title, author, brand, category, description, price, "
        "stock_quantity, cover_class, is_featured, is_active, image_url FROM products ORDER BY id DESC";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddStringToObject(err, "error", sqlite3_errmsg(g_db));
        char *s = cJSON_PrintUnformatted(err);
        cJSON_Delete(err);
        return s;
    }

    cJSON *arr = cJSON_CreateArray();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(p, "sku", (const char*)sqlite3_column_text(stmt, 1));
        const char *type = (const char*)sqlite3_column_text(stmt, 2);
        cJSON_AddStringToObject(p, "type", type ? type : "");
        cJSON_AddStringToObject(p, "title", (const char*)sqlite3_column_text(stmt, 3));
        const char *author = (const char*)sqlite3_column_text(stmt, 4);
        const char *brand = (const char*)sqlite3_column_text(stmt, 5);
        if (author) cJSON_AddStringToObject(p, "author", author);
        if (brand) cJSON_AddStringToObject(p, "brand", brand);
        const char *cat = (const char*)sqlite3_column_text(stmt, 6);
        cJSON_AddStringToObject(p, "category", cat ? cat : "");
        const char *desc = (const char*)sqlite3_column_text(stmt, 7);
        cJSON_AddStringToObject(p, "description", desc ? desc : "");
        cJSON_AddNumberToObject(p, "price", sqlite3_column_double(stmt, 8));
        cJSON_AddNumberToObject(p, "stock_quantity", sqlite3_column_int(stmt, 9));
        const char *cov = (const char*)sqlite3_column_text(stmt, 10);
        cJSON_AddStringToObject(p, "cover_class", cov ? cov : "cover-book-1");
        cJSON_AddBoolToObject(p, "is_featured", sqlite3_column_int(stmt, 11));
        cJSON_AddBoolToObject(p, "is_active", sqlite3_column_int(stmt, 12));
        const char *img_url = (const char*)sqlite3_column_text(stmt, 13);
        cJSON_AddStringToObject(p, "image_url", img_url ? img_url : "");
        cJSON_AddItemToArray(arr, p);
    }
    sqlite3_finalize(stmt);

    char *out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}

char *db_admin_create_product_json(const char *json_body) {
    cJSON *resp = cJSON_CreateObject();
    if (!json_body || strlen(json_body) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Request body is empty");
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    cJSON *root = cJSON_Parse(json_body);
    if (!root) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid JSON syntax");
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    cJSON *title_item = cJSON_GetObjectItem(root, "title");
    if (!title_item || !title_item->valuestring || strlen(title_item->valuestring) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Title is required and cannot be empty");
        cJSON_Delete(root);
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    cJSON *cat_item = cJSON_GetObjectItem(root, "category");
    if (!cat_item || !cat_item->valuestring || strlen(cat_item->valuestring) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Category is required and cannot be empty");
        cJSON_Delete(root);
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    cJSON *price_item = cJSON_GetObjectItem(root, "price");
    if (!price_item || !cJSON_IsNumber(price_item) || price_item->valuedouble <= 0.0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Price must be a positive number greater than 0");
        cJSON_Delete(root);
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    cJSON *stock_item = cJSON_GetObjectItem(root, "stock_quantity");
    int stock_val = 10;
    if (stock_item && cJSON_IsNumber(stock_item)) {
        stock_val = stock_item->valueint;
        if (stock_val < 0) {
            cJSON_AddStringToObject(resp, "status", "error");
            cJSON_AddStringToObject(resp, "message", "Stock quantity cannot be negative");
            cJSON_Delete(root);
            char *out = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return out;
        }
    }

    const char *prod_type = "book";
    cJSON *type_item = cJSON_GetObjectItem(root, "type");
    if (type_item && type_item->valuestring && strlen(type_item->valuestring) > 0) {
        prod_type = type_item->valuestring;
        if (strcmp(prod_type, "book") != 0 && strcmp(prod_type, "stationery") != 0) {
            cJSON_AddStringToObject(resp, "status", "error");
            cJSON_AddStringToObject(resp, "message", "Product type must be either 'book' or 'stationery'");
            cJSON_Delete(root);
            char *out = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return out;
        }
    }

    /* Auto-generate unique SKU if not provided */
    char final_sku[64] = "";
    cJSON *sku_item = cJSON_GetObjectItem(root, "sku");
    if (sku_item && sku_item->valuestring && strlen(sku_item->valuestring) > 0) {
        strncpy(final_sku, sku_item->valuestring, sizeof(final_sku) - 1);
    } else {
        sqlite3_stmt *sku_stmt;
        int max_id = 0;
        if (sqlite3_prepare_v2(g_db, "SELECT MAX(id) FROM products", -1, &sku_stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(sku_stmt) == SQLITE_ROW) {
                max_id = sqlite3_column_int(sku_stmt, 0);
            }
            sqlite3_finalize(sku_stmt);
        }
        char prefix = (strcmp(prod_type, "stationery") == 0) ? 'S' : 'B';
        snprintf(final_sku, sizeof(final_sku), "%c%03d", prefix, max_id + 1);

        /* Ensure uniqueness */
        sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM products WHERE sku = ?", -1, &sku_stmt, NULL);
        sqlite3_bind_text(sku_stmt, 1, final_sku, -1, SQLITE_STATIC);
        int exists = 0;
        if (sqlite3_step(sku_stmt) == SQLITE_ROW) exists = sqlite3_column_int(sku_stmt, 0);
        sqlite3_finalize(sku_stmt);
        if (exists > 0) {
            snprintf(final_sku, sizeof(final_sku), "%c%03d-%u", prefix, max_id + 1, (unsigned)(time(NULL) % 1000));
        }
    }

    cJSON *author_item = cJSON_GetObjectItem(root, "author");
    const char *author_val = (author_item && author_item->valuestring) ? author_item->valuestring : "";

    cJSON *isbn_item = cJSON_GetObjectItem(root, "isbn");
    const char *isbn_val = (isbn_item && isbn_item->valuestring) ? isbn_item->valuestring : "";

    cJSON *pub_item = cJSON_GetObjectItem(root, "publisher");
    const char *pub_val = (pub_item && pub_item->valuestring) ? pub_item->valuestring : "";

    cJSON *ed_item = cJSON_GetObjectItem(root, "edition");
    const char *ed_val = (ed_item && ed_item->valuestring) ? ed_item->valuestring : "";

    cJSON *lang_item = cJSON_GetObjectItem(root, "language");
    const char *lang_val = (lang_item && lang_item->valuestring) ? lang_item->valuestring : "English";

    cJSON *brand_item = cJSON_GetObjectItem(root, "brand");
    const char *brand_val = (brand_item && brand_item->valuestring) ? brand_item->valuestring : "";

    cJSON *color_item = cJSON_GetObjectItem(root, "color");
    const char *color_val = (color_item && color_item->valuestring) ? color_item->valuestring : "";

    cJSON *size_item = cJSON_GetObjectItem(root, "size");
    const char *size_val = (size_item && size_item->valuestring) ? size_item->valuestring : "";

    cJSON *specs_item = cJSON_GetObjectItem(root, "specifications");
    const char *specs_val = (specs_item && specs_item->valuestring) ? specs_item->valuestring : "";

    cJSON *desc_item = cJSON_GetObjectItem(root, "description");
    const char *desc_val = (desc_item && desc_item->valuestring) ? desc_item->valuestring : "";

    cJSON *disc_item = cJSON_GetObjectItem(root, "discount_price");
    double disc_val = (disc_item && cJSON_IsNumber(disc_item)) ? disc_item->valuedouble : 0.0;

    cJSON *cov_item = cJSON_GetObjectItem(root, "cover_class");
    const char *cov_val = (cov_item && cov_item->valuestring && strlen(cov_item->valuestring) > 0)
                            ? cov_item->valuestring
                            : ((strcmp(prod_type, "stationery") == 0) ? "cover-stat-1" : "cover-book-1");

    cJSON *feat_item = cJSON_GetObjectItem(root, "is_featured");
    int feat_val = (feat_item && cJSON_IsBool(feat_item)) ? (cJSON_IsTrue(feat_item) ? 1 : 0) : 0;

    cJSON *img_item = cJSON_GetObjectItem(root, "image_url");
    const char *img_val = (img_item && img_item->valuestring) ? img_item->valuestring : "";

    const char *ins_sql = 
        "INSERT INTO products (sku, type, title, author, isbn, publisher, edition, language, "
        "brand, color, size, specifications, category, description, price, discount_price, "
        "stock_quantity, cover_class, is_featured, is_active, image_url) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 1, ?)";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(g_db, ins_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", sqlite3_errmsg(g_db));
        cJSON_Delete(root);
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    sqlite3_bind_text(stmt, 1, final_sku, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, prod_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, title_item->valuestring, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, author_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, isbn_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, pub_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, ed_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, lang_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, brand_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, color_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, size_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, specs_val, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 13, cat_item->valuestring, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 14, desc_val, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 15, price_item->valuedouble);
    sqlite3_bind_double(stmt, 16, disc_val);
    sqlite3_bind_int(stmt, 17, stock_val);
    sqlite3_bind_text(stmt, 18, cov_val, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 19, feat_val);
    sqlite3_bind_text(stmt, 20, img_val, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    cJSON_Delete(root);

    if (rc == SQLITE_DONE) {
        int new_id = (int)sqlite3_last_insert_rowid(g_db);
        cJSON_AddStringToObject(resp, "status", "success");
        cJSON_AddStringToObject(resp, "message", "Product created successfully");
        cJSON_AddNumberToObject(resp, "id", new_id);
        cJSON_AddStringToObject(resp, "sku", final_sku);
    } else {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", sqlite3_errmsg(g_db));
    }

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

char *db_admin_update_product_json(int id, const char *json_body) {
    cJSON *resp = cJSON_CreateObject();
    if (!json_body || strlen(json_body) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Request body is empty");
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    cJSON *root = cJSON_Parse(json_body);
    if (!root) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid JSON syntax");
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    /* Retrieve existing product record */
    const char *sel_sql = 
        "SELECT sku, type, title, author, isbn, publisher, edition, language, brand, "
        "color, size, specifications, category, description, price, discount_price, "
        "stock_quantity, cover_class, is_featured, is_active, image_url FROM products WHERE id = ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, sel_sql, -1, &stmt, NULL) != SQLITE_OK) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Database error");
        cJSON_Delete(root);
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }
    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Product not found");
        cJSON_Delete(root);
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    /* Read current values into local buffers */
    char cur_sku[64] = "";
    char cur_type[32] = "";
    char cur_title[256] = "";
    char cur_author[128] = "";
    char cur_isbn[64] = "";
    char cur_pub[128] = "";
    char cur_ed[64] = "";
    char cur_lang[32] = "";
    char cur_brand[128] = "";
    char cur_color[64] = "";
    char cur_size[64] = "";
    char cur_specs[256] = "";
    char cur_cat[128] = "";
    char cur_desc[1024] = "";
    char cur_cov[64] = "";
    char cur_img[256] = "";

    if (sqlite3_column_text(stmt, 0)) strncpy(cur_sku, (const char*)sqlite3_column_text(stmt, 0), sizeof(cur_sku) - 1);
    if (sqlite3_column_text(stmt, 1)) strncpy(cur_type, (const char*)sqlite3_column_text(stmt, 1), sizeof(cur_type) - 1);
    if (sqlite3_column_text(stmt, 2)) strncpy(cur_title, (const char*)sqlite3_column_text(stmt, 2), sizeof(cur_title) - 1);
    if (sqlite3_column_text(stmt, 3)) strncpy(cur_author, (const char*)sqlite3_column_text(stmt, 3), sizeof(cur_author) - 1);
    if (sqlite3_column_text(stmt, 4)) strncpy(cur_isbn, (const char*)sqlite3_column_text(stmt, 4), sizeof(cur_isbn) - 1);
    if (sqlite3_column_text(stmt, 5)) strncpy(cur_pub, (const char*)sqlite3_column_text(stmt, 5), sizeof(cur_pub) - 1);
    if (sqlite3_column_text(stmt, 6)) strncpy(cur_ed, (const char*)sqlite3_column_text(stmt, 6), sizeof(cur_ed) - 1);
    if (sqlite3_column_text(stmt, 7)) strncpy(cur_lang, (const char*)sqlite3_column_text(stmt, 7), sizeof(cur_lang) - 1);
    if (sqlite3_column_text(stmt, 8)) strncpy(cur_brand, (const char*)sqlite3_column_text(stmt, 8), sizeof(cur_brand) - 1);
    if (sqlite3_column_text(stmt, 9)) strncpy(cur_color, (const char*)sqlite3_column_text(stmt, 9), sizeof(cur_color) - 1);
    if (sqlite3_column_text(stmt, 10)) strncpy(cur_size, (const char*)sqlite3_column_text(stmt, 10), sizeof(cur_size) - 1);
    if (sqlite3_column_text(stmt, 11)) strncpy(cur_specs, (const char*)sqlite3_column_text(stmt, 11), sizeof(cur_specs) - 1);
    if (sqlite3_column_text(stmt, 12)) strncpy(cur_cat, (const char*)sqlite3_column_text(stmt, 12), sizeof(cur_cat) - 1);
    if (sqlite3_column_text(stmt, 13)) strncpy(cur_desc, (const char*)sqlite3_column_text(stmt, 13), sizeof(cur_desc) - 1);
    double cur_price = sqlite3_column_double(stmt, 14);
    double cur_disc = sqlite3_column_double(stmt, 15);
    int cur_stock = sqlite3_column_int(stmt, 16);
    if (sqlite3_column_text(stmt, 17)) strncpy(cur_cov, (const char*)sqlite3_column_text(stmt, 17), sizeof(cur_cov) - 1);
    int cur_feat = sqlite3_column_int(stmt, 18);
    int cur_active = sqlite3_column_int(stmt, 19);
    if (sqlite3_column_text(stmt, 20)) strncpy(cur_img, (const char*)sqlite3_column_text(stmt, 20), sizeof(cur_img) - 1);

    sqlite3_finalize(stmt);

    /* Apply overrides and validate */
    cJSON *title_item = cJSON_GetObjectItem(root, "title");
    if (title_item && title_item->valuestring) {
        if (strlen(title_item->valuestring) == 0) {
            cJSON_AddStringToObject(resp, "status", "error");
            cJSON_AddStringToObject(resp, "message", "Title cannot be empty");
            cJSON_Delete(root);
            char *out = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return out;
        }
        strncpy(cur_title, title_item->valuestring, sizeof(cur_title) - 1);
    }

    cJSON *cat_item = cJSON_GetObjectItem(root, "category");
    if (cat_item && cat_item->valuestring && strlen(cat_item->valuestring) > 0) {
        strncpy(cur_cat, cat_item->valuestring, sizeof(cur_cat) - 1);
    }

    cJSON *price_item = cJSON_GetObjectItem(root, "price");
    if (price_item && cJSON_IsNumber(price_item)) {
        if (price_item->valuedouble < 0.0) {
            cJSON_AddStringToObject(resp, "status", "error");
            cJSON_AddStringToObject(resp, "message", "Price cannot be negative");
            cJSON_Delete(root);
            char *out = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return out;
        }
        cur_price = price_item->valuedouble;
    }

    cJSON *disc_item = cJSON_GetObjectItem(root, "discount_price");
    if (disc_item && cJSON_IsNumber(disc_item)) {
        if (disc_item->valuedouble < 0.0) {
            cJSON_AddStringToObject(resp, "status", "error");
            cJSON_AddStringToObject(resp, "message", "Discount price cannot be negative");
            cJSON_Delete(root);
            char *out = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return out;
        }
        cur_disc = disc_item->valuedouble;
    }

    cJSON *stock_item = cJSON_GetObjectItem(root, "stock_quantity");
    if (stock_item && cJSON_IsNumber(stock_item)) {
        if (stock_item->valueint < 0) {
            cJSON_AddStringToObject(resp, "status", "error");
            cJSON_AddStringToObject(resp, "message", "Stock quantity cannot be negative");
            cJSON_Delete(root);
            char *out = cJSON_PrintUnformatted(resp);
            cJSON_Delete(resp);
            return out;
        }
        cur_stock = stock_item->valueint;
    }

    cJSON *type_item = cJSON_GetObjectItem(root, "type");
    if (type_item && type_item->valuestring && strlen(type_item->valuestring) > 0) {
        if (strcmp(type_item->valuestring, "book") == 0 || strcmp(type_item->valuestring, "stationery") == 0) {
            strncpy(cur_type, type_item->valuestring, sizeof(cur_type) - 1);
        }
    }

    cJSON *author_item = cJSON_GetObjectItem(root, "author");
    if (author_item && author_item->valuestring) strncpy(cur_author, author_item->valuestring, sizeof(cur_author) - 1);

    cJSON *isbn_item = cJSON_GetObjectItem(root, "isbn");
    if (isbn_item && isbn_item->valuestring) strncpy(cur_isbn, isbn_item->valuestring, sizeof(cur_isbn) - 1);

    cJSON *pub_item = cJSON_GetObjectItem(root, "publisher");
    if (pub_item && pub_item->valuestring) strncpy(cur_pub, pub_item->valuestring, sizeof(cur_pub) - 1);

    cJSON *ed_item = cJSON_GetObjectItem(root, "edition");
    if (ed_item && ed_item->valuestring) strncpy(cur_ed, ed_item->valuestring, sizeof(cur_ed) - 1);

    cJSON *brand_item = cJSON_GetObjectItem(root, "brand");
    if (brand_item && brand_item->valuestring) strncpy(cur_brand, brand_item->valuestring, sizeof(cur_brand) - 1);

    cJSON *desc_item = cJSON_GetObjectItem(root, "description");
    if (desc_item && desc_item->valuestring) strncpy(cur_desc, desc_item->valuestring, sizeof(cur_desc) - 1);

    cJSON *cov_item = cJSON_GetObjectItem(root, "cover_class");
    if (cov_item && cov_item->valuestring && strlen(cov_item->valuestring) > 0) {
        strncpy(cur_cov, cov_item->valuestring, sizeof(cur_cov) - 1);
    }

    cJSON *feat_item = cJSON_GetObjectItem(root, "is_featured");
    if (feat_item) {
        if (cJSON_IsBool(feat_item)) cur_feat = cJSON_IsTrue(feat_item) ? 1 : 0;
        else if (cJSON_IsNumber(feat_item)) cur_feat = feat_item->valueint ? 1 : 0;
    }

    cJSON *act_item = cJSON_GetObjectItem(root, "is_active");
    if (act_item) {
        if (cJSON_IsBool(act_item)) cur_active = cJSON_IsTrue(act_item) ? 1 : 0;
        else if (cJSON_IsNumber(act_item)) cur_active = act_item->valueint ? 1 : 0;
    }

    cJSON *img_item = cJSON_GetObjectItem(root, "image_url");
    if (img_item && img_item->valuestring) {
        strncpy(cur_img, img_item->valuestring, sizeof(cur_img) - 1);
    }

    const char *upd_sql = 
        "UPDATE products SET sku = ?, type = ?, title = ?, author = ?, isbn = ?, "
        "publisher = ?, edition = ?, language = ?, brand = ?, color = ?, size = ?, "
        "specifications = ?, category = ?, description = ?, price = ?, discount_price = ?, "
        "stock_quantity = ?, cover_class = ?, is_featured = ?, is_active = ?, image_url = ? WHERE id = ?";

    sqlite3_stmt *upd_stmt;
    int rc = sqlite3_prepare_v2(g_db, upd_sql, -1, &upd_stmt, NULL);
    if (rc != SQLITE_OK) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", sqlite3_errmsg(g_db));
        cJSON_Delete(root);
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }

    sqlite3_bind_text(upd_stmt, 1, cur_sku, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 2, cur_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 3, cur_title, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 4, cur_author, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 5, cur_isbn, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 6, cur_pub, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 7, cur_ed, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 8, cur_lang, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 9, cur_brand, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 10, cur_color, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 11, cur_size, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 12, cur_specs, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 13, cur_cat, -1, SQLITE_STATIC);
    sqlite3_bind_text(upd_stmt, 14, cur_desc, -1, SQLITE_STATIC);
    sqlite3_bind_double(upd_stmt, 15, cur_price);
    sqlite3_bind_double(upd_stmt, 16, cur_disc);
    sqlite3_bind_int(upd_stmt, 17, cur_stock);
    sqlite3_bind_text(upd_stmt, 18, cur_cov, -1, SQLITE_STATIC);
    sqlite3_bind_int(upd_stmt, 19, cur_feat);
    sqlite3_bind_int(upd_stmt, 20, cur_active);
    sqlite3_bind_text(upd_stmt, 21, cur_img, -1, SQLITE_STATIC);
    sqlite3_bind_int(upd_stmt, 22, id);

    rc = sqlite3_step(upd_stmt);
    sqlite3_finalize(upd_stmt);
    cJSON_Delete(root);

    if (rc == SQLITE_DONE) {
        cJSON_AddStringToObject(resp, "status", "success");
        cJSON_AddStringToObject(resp, "message", "Product updated successfully");
        cJSON_AddNumberToObject(resp, "id", id);
        cJSON_AddNumberToObject(resp, "stock_quantity", cur_stock);
        cJSON_AddNumberToObject(resp, "price", cur_price);
        cJSON_AddBoolToObject(resp, "is_active", cur_active);
    } else {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", sqlite3_errmsg(g_db));
    }

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

char *db_admin_delete_product_json(int id) {
    cJSON *resp = cJSON_CreateObject();

    /* Verify product exists */
    sqlite3_stmt *chk_stmt;
    sqlite3_prepare_v2(g_db, "SELECT id, title, is_active FROM products WHERE id = ?", -1, &chk_stmt, NULL);
    sqlite3_bind_int(chk_stmt, 1, id);
    if (sqlite3_step(chk_stmt) != SQLITE_ROW) {
        sqlite3_finalize(chk_stmt);
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Product not found");
        char *out = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return out;
    }
    int cur_active = sqlite3_column_int(chk_stmt, 2);
    sqlite3_finalize(chk_stmt);

    /* Check if product is referenced in order_items */
    sqlite3_stmt *ref_stmt;
    sqlite3_prepare_v2(g_db, "SELECT COUNT(*) FROM order_items WHERE product_id = ?", -1, &ref_stmt, NULL);
    sqlite3_bind_int(ref_stmt, 1, id);
    int ref_count = 0;
    if (sqlite3_step(ref_stmt) == SQLITE_ROW) {
        ref_count = sqlite3_column_int(ref_stmt, 0);
    }
    sqlite3_finalize(ref_stmt);

    if (ref_count > 0) {
        /* If associated with orders, toggle or deactivate to preserve foreign key relational integrity */
        int next_active = (cur_active == 1) ? 0 : 1;
        sqlite3_stmt *upd_stmt;
        sqlite3_prepare_v2(g_db, "UPDATE products SET is_active = ? WHERE id = ?", -1, &upd_stmt, NULL);
        sqlite3_bind_int(upd_stmt, 1, next_active);
        sqlite3_bind_int(upd_stmt, 2, id);
        sqlite3_step(upd_stmt);
        sqlite3_finalize(upd_stmt);

        cJSON_AddStringToObject(resp, "status", "success");
        cJSON_AddStringToObject(resp, "message", (next_active == 0) ? "Product deactivated (referenced in order history)" : "Product reactivated");
        cJSON_AddBoolToObject(resp, "is_active", next_active);
    } else {
        /* No orders reference this product; perform safe deletion */
        sqlite3_stmt *del_stmt;
        sqlite3_prepare_v2(g_db, "DELETE FROM products WHERE id = ?", -1, &del_stmt, NULL);
        sqlite3_bind_int(del_stmt, 1, id);
        int rc = sqlite3_step(del_stmt);
        sqlite3_finalize(del_stmt);

        if (rc == SQLITE_DONE) {
            cJSON_AddStringToObject(resp, "status", "success");
            cJSON_AddStringToObject(resp, "message", "Product deleted successfully");
            cJSON_AddBoolToObject(resp, "deleted", true);
        } else {
            /* Fallback to deactivation */
            sqlite3_stmt *upd_stmt;
            sqlite3_prepare_v2(g_db, "UPDATE products SET is_active = 0 WHERE id = ?", -1, &upd_stmt, NULL);
            sqlite3_bind_int(upd_stmt, 1, id);
            sqlite3_step(upd_stmt);
            sqlite3_finalize(upd_stmt);

            cJSON_AddStringToObject(resp, "status", "success");
            cJSON_AddStringToObject(resp, "message", "Product deactivated");
            cJSON_AddBoolToObject(resp, "is_active", 0);
        }
    }

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* --- Secure Admin Authentication Implementation --- */

int db_admin_is_setup_completed(void) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id FROM users WHERE role = 'admin' AND password_hash IS NOT NULL AND length(password_hash) > 0 LIMIT 1";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    int completed = (sqlite3_step(stmt) == SQLITE_ROW) ? 1 : 0;
    sqlite3_finalize(stmt);
    return completed;
}

char *db_admin_setup_json(const char *email, const char *password) {
    cJSON *resp = cJSON_CreateObject();
    if (db_admin_is_setup_completed()) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Admin setup has already been completed. First-time setup is disabled.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    if (!password || strlen(password) < 6) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Admin password must be at least 6 characters long.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    const char *admin_email = (email && strlen(email) > 0) ? email : "admin@brishtilibrary.edu";

    char salt[33];
    char hash[65];
    sha256_generate_salt(salt);
    sha256_hash_password(password, salt, hash);

    /* Check if an admin record exists to update, or create a new one */
    sqlite3_stmt *stmt;
    const char *sel_sql = "SELECT id FROM users WHERE role = 'admin' LIMIT 1";
    sqlite3_prepare_v2(g_db, sel_sql, -1, &stmt, NULL);
    int admin_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        admin_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (admin_id > 0) {
        const char *upd_sql = "UPDATE users SET email = ?, password_hash = ?, password_salt = ? WHERE id = ?";
        sqlite3_prepare_v2(g_db, upd_sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, admin_email, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, salt, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, admin_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        const char *ins_sql = "INSERT INTO users (email, name, role, password_hash, password_salt) VALUES (?, 'Brishti Admin', 'admin', ?, ?)";
        sqlite3_prepare_v2(g_db, ins_sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, admin_email, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, salt, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        admin_id = (int)sqlite3_last_insert_rowid(g_db);
    }

    char *token = db_create_session(admin_id);
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Admin password configured successfully. Setup is now permanently closed.");
    cJSON_AddStringToObject(resp, "token", token ? token : "");
    cJSON_AddStringToObject(resp, "role", "admin");
    cJSON_AddStringToObject(resp, "email", admin_email);
    if (token) free(token);

    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

char *db_admin_login_json(const char *email, const char *password, char *out_token_buf, size_t token_buf_len) {
    if (out_token_buf && token_buf_len > 0) out_token_buf[0] = '\0';
    cJSON *resp = cJSON_CreateObject();

    if (!db_admin_is_setup_completed()) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Admin account not configured yet. Please complete initial setup first.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    if (!password || strlen(password) == 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Password is required.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, email, name, password_hash, password_salt FROM users WHERE role = 'admin' AND password_hash IS NOT NULL";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Database error.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    int found = 0;
    int uid = 0;
    char db_name[64] = "Brishti Admin";
    char db_email[64] = "admin@brishtilibrary.edu";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *row_email = (const char*)sqlite3_column_text(stmt, 1);
        const char *db_hash = (const char*)sqlite3_column_text(stmt, 3);
        const char *db_salt = (const char*)sqlite3_column_text(stmt, 4);

        if (email && strlen(email) > 0 && row_email && strcmp(email, row_email) != 0) {
            continue;
        }

        if (db_hash && db_salt) {
            char computed_hash[65];
            sha256_hash_password(password, db_salt, computed_hash);
            if (strcmp(computed_hash, db_hash) == 0) {
                found = 1;
                uid = sqlite3_column_int(stmt, 0);
                if (sqlite3_column_text(stmt, 2)) strncpy(db_name, (const char*)sqlite3_column_text(stmt, 2), sizeof(db_name) - 1);
                if (row_email) strncpy(db_email, row_email, sizeof(db_email) - 1);
                break;
            }
        }
    }
    sqlite3_finalize(stmt);

    if (!found) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Invalid admin credentials.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    char *token = db_create_session(uid);
    if (token && out_token_buf && token_buf_len > 0) {
        strncpy(out_token_buf, token, token_buf_len - 1);
        out_token_buf[token_buf_len - 1] = '\0';
    }

    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Admin authentication successful.");
    cJSON_AddStringToObject(resp, "token", token ? token : "");
    cJSON_AddStringToObject(resp, "role", "admin");
    cJSON_AddStringToObject(resp, "name", db_name);
    cJSON_AddStringToObject(resp, "email", db_email);
    if (token) free(token);

    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

char *db_admin_change_password_json(int admin_user_id, const char *current_password, const char *new_password) {
    cJSON *resp = cJSON_CreateObject();
    if (admin_user_id <= 0) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Admin authorization required.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    if (!current_password || !new_password || strlen(new_password) < 6) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "New password must be at least 6 characters.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    sqlite3_stmt *stmt;
    const char *sel_sql = "SELECT password_hash, password_salt FROM users WHERE id = ? AND role = 'admin'";
    if (sqlite3_prepare_v2(g_db, sel_sql, -1, &stmt, NULL) != SQLITE_OK) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Database error.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }
    sqlite3_bind_int(stmt, 1, admin_user_id);

    int valid = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *db_hash = (const char*)sqlite3_column_text(stmt, 0);
        const char *db_salt = (const char*)sqlite3_column_text(stmt, 1);
        if (db_hash && db_salt) {
            char computed[65];
            sha256_hash_password(current_password, db_salt, computed);
            if (strcmp(computed, db_hash) == 0) {
                valid = 1;
            }
        }
    }
    sqlite3_finalize(stmt);

    if (!valid) {
        cJSON_AddStringToObject(resp, "status", "error");
        cJSON_AddStringToObject(resp, "message", "Current password is incorrect.");
        char *s = cJSON_PrintUnformatted(resp);
        cJSON_Delete(resp);
        return s;
    }

    char new_salt[33];
    char new_hash[65];
    sha256_generate_salt(new_salt);
    sha256_hash_password(new_password, new_salt, new_hash);

    const char *upd_sql = "UPDATE users SET password_hash = ?, password_salt = ? WHERE id = ?";
    sqlite3_prepare_v2(g_db, upd_sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, new_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, new_salt, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, admin_user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "message", "Password changed successfully.");
    char *s = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return s;
}

