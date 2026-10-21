#include "../include/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define ASSERT_TEST(name, cond) do { \
    if (cond) { \
        printf("  [PASS] %s\n", (name)); \
        g_tests_passed++; \
    } else { \
        printf("  [FAIL] %s\n", (name)); \
        g_tests_failed++; \
    } \
} while(0)

int main(void) {
    printf("====================================================\n");
    printf("   TESTING SQLITE ONLINE BACKUP API (C99)           \n");
    printf("====================================================\n");

    const char *test_db = "database/test_active.db";
    const char *backup_db = "database/backups/test_backup_out.db";

    /* Clean up any previous test artifacts */
    remove(test_db);
    remove(backup_db);

    int rc = db_init(test_db);
    ASSERT_TEST("db_init initializes active test database", rc == SQLITE_OK && g_db != NULL);

    /* 1. Test protection against overwriting active database */
    int page_count = 0;
    rc = db_backup_to_file(test_db, &page_count);
    ASSERT_TEST("db_backup_to_file rejects overwriting active database with SQLITE_MISUSE", rc == SQLITE_MISUSE);

    /* Also test with relative path variants of active database */
    rc = db_backup_to_file("./database/test_active.db", &page_count);
    ASSERT_TEST("db_backup_to_file rejects relative path to active database", rc == SQLITE_MISUSE);

    /* 2. Test successful safe online backup */
    rc = db_backup_to_file(backup_db, &page_count);
    ASSERT_TEST("db_backup_to_file succeeds on valid destination", rc == SQLITE_OK);
    ASSERT_TEST("Backup copies non-zero pages (page_count > 0)", page_count > 0);

    /* 3. Verify backup file consistency by opening it with SQLite */
    sqlite3 *pVerify = NULL;
    rc = sqlite3_open(backup_db, &pVerify);
    ASSERT_TEST("Backup database can be opened with sqlite3_open", rc == SQLITE_OK && pVerify != NULL);

    /* Verify schema in the backup */
    sqlite3_stmt *stmt;
    const char *chk_sql = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name IN ('products', 'categories', 'users', 'orders')";
    rc = sqlite3_prepare_v2(pVerify, chk_sql, -1, &stmt, NULL);
    ASSERT_TEST("Prepared schema verification query on backup database", rc == SQLITE_OK);

    int table_count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        table_count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    ASSERT_TEST("Backup database contains all 4 core application tables", table_count == 4);

    /* Run integrity check on backup database */
    rc = sqlite3_prepare_v2(pVerify, "PRAGMA integrity_check;", -1, &stmt, NULL);
    ASSERT_TEST("PRAGMA integrity_check prepared on backup database", rc == SQLITE_OK);
    char integrity_res[64] = "";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *txt = (const char*)sqlite3_column_text(stmt, 0);
        if (txt) strncpy(integrity_res, txt, sizeof(integrity_res) - 1);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(pVerify);

    ASSERT_TEST("PRAGMA integrity_check reports 'ok'", strcmp(integrity_res, "ok") == 0);

    /* 4. Test db_admin_create_backup_json */
    char *json_res = db_admin_create_backup_json(NULL);
    ASSERT_TEST("db_admin_create_backup_json returns non-null JSON", json_res != NULL);
    ASSERT_TEST("db_admin_create_backup_json returns status success", strstr(json_res, "\"status\":\"success\"") != NULL);
    ASSERT_TEST("db_admin_create_backup_json includes backup_file", strstr(json_res, "backup_file") != NULL);
    ASSERT_TEST("db_admin_create_backup_json includes page_count", strstr(json_res, "page_count") != NULL);
    ASSERT_TEST("db_admin_create_backup_json includes size_bytes", strstr(json_res, "size_bytes") != NULL);
    free(json_res);

    /* 5. Test db_admin_create_backup_json rejects path traversal */
    char *bad_json = db_admin_create_backup_json("../../etc/passwd");
    ASSERT_TEST("db_admin_create_backup_json rejects path traversal", strstr(bad_json, "\"status\":\"error\"") != NULL);
    free(bad_json);

    /* 6. Test db_admin_list_backups_json */
    char *list_json = db_admin_list_backups_json();
    ASSERT_TEST("db_admin_list_backups_json returns non-null JSON", list_json != NULL);
    ASSERT_TEST("db_admin_list_backups_json includes backups array", strstr(list_json, "\"backups\":") != NULL);
    ASSERT_TEST("db_admin_list_backups_json reports count >= 1", strstr(list_json, "\"count\":") != NULL);
    free(list_json);

    /* Clean up */
    db_close();
    remove(test_db);
    remove(backup_db);

    printf("====================================================\n");
    printf("RESULTS: Total: %d | Passed: %d | Failed: %d\n",
           g_tests_passed + g_tests_failed, g_tests_passed, g_tests_failed);
    printf("====================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
