# BrishtiLibrary — Project Progress & CSE214 Verification Report

**Project Name:** BrishtiLibrary (বৃষ্টি লাইব্রেরি)  
**Course:** CSE214 — Algorithm Lab & Full-Stack Development  
**Academic Requirement:** Backend strictly in C (C99), SQLite via C API, HTML5/CSS3/Vanilla JS Frontend.  
**Report Date:** October 22, 2026  
**Status:** All Core Storefront, CSE214 Algorithms, Admin Product CRUD, Secure Admin Email Login, Category Management, Photo Uploads, Warm Blue Themes, SQLite Online Backup API, and Cross-Platform Linux/Docker Deployment verified.

---

## 1. Executive Summary

BrishtiLibrary is a full-featured e-commerce platform for physical books and stationery that simultaneously powers interactive laboratory demonstrations for the **CSE214 Algorithm Course**.

All core requirements—including student customer journeys, product discovery, shopping cart calculations, transactional order placement, order status tracking, admin catalog CRUD, and 5 classic algorithmic paradigms—have been implemented strictly in **C** using SQLite C API (`sqlite3.h`), and `cJSON`.

In this milestone, **Email-Based Admin Authentication** was fully implemented and verified:
1. **Email-Based Admin Credentials:** Administrator setup and login strictly require a valid email address and password.
2. **Backend Validation:** Validates email format on the C backend; rejects missing or malformed emails with HTTP 400.
3. **Parameterized Case-Insensitive Queries:** Email lookups in SQLite use parameterized statements (`lower(email) = lower(?)`) preventing SQL injection and allowing case-insensitive login.
4. **Constant-Time Verification:** Hash matching uses constant-time comparison (`constant_time_str_equals`) to prevent timing side-channels.
5. **Role-Based Authorization:** Server-side authorization gates every `/api/admin/*` endpoint against the `sessions` table. Customers cannot gain admin privileges.
6. **No Leaked Credentials:** Password hashes and salts are never returned in JSON API responses. Passwords are never saved in `localStorage` or `sessionStorage`.
7. **Clean First-Time Setup:** Preserves the one-time admin setup feature, permanently locking further setup calls upon completion.

---

## 2. Test Suite & Verification Results

### Summary of Verified Tests:
* **CSE214 Algorithm Unit Suite (`test_algorithms.exe`):** 76 / 76 Passed (100%)
* **Standalone SQLite Online Backup C99 Suite (`test_backup.exe`):** 19 / 19 Passed (100%)
* **Web & Security Integration Suite (`test_new_features.ps1`):** 60 / 60 Passed (100%)
* **Grand Total Verified Tests:** **155 / 155 Passed (100%)**

---

### A. Web & Security Integration Suite (`test_new_features.ps1`)
* **Total Tests Executed:** 60
* **Tests Passed:** 60 (100%)
* **Tests Failed:** 0
* **Execution Time:** ~4 seconds

| Module | Features Tested | Result |
| :--- | :--- | :---: |
| **Admin Setup & Auth** | Setup status, demo admin rejection (403), customer demo login (200), setup missing email rejection (400), setup invalid email syntax rejection (400), first-time setup with valid email, response email verification, second setup lockout (400), login missing email rejection (400), login wrong email rejection (401), login wrong password rejection (401), login correct email (200), case-insensitive email login (`Admin@BrishtiLibrary.EDU`), password change, new password email login. | **18/18 PASS** |
| **Category Management** | Public category query, admin category creation, admin category rename, deletion guard on active products (400), safe deletion of empty category. | **5/5 PASS** |
| **Product Image Uploads** | PNG magic byte validation, URL generation (`/uploads/...`), disk file persistence, static image serving (200), non-image rejection (400), product creation with image, catalog query photo retention. | **8/8 PASS** |
| **Warm Blue Themes** | CSS variables for Light (`#EAF2F8`) and Dark (`#151F2B`) palettes, theme manager script, sun/moon toggles across Storefront, Admin, and Algorithm Lab. | **14/14 PASS** |
| **SQLite Online Backup** | Unauthorized customer rejection (403), admin backup creation (201), filename formatting, file size verification, physical disk check in `database/backups/`, GET backups list, GET backup download (`application/x-sqlite3`), path traversal rejection (403), DELETE backup (200). | **9/9 PASS** |
| **Algorithm Regressions** | Quick Sort C API microsecond timing, Dijkstra campus graph shortest path API. | **2/2 PASS** |
| **Active DB Clean Reset** | Reset script ensures pristine first-time admin setup state for the user. | **PASS** |

---

### B. Standalone Test Suites
* **CSE214 Algorithm Suite (`test_algorithms.exe`):** 76 / 76 Passed (100%)
* **Standalone SQLite Online Backup C99 Suite (`test_backup.exe`):** 19 / 19 Passed (100%)

---

## 3. Security Analysis & Limitations

* **Hashing Scheme:** 5,000 iterative rounds of SHA-256 with 32-character pseudo-random salt and constant-time string comparison (`constant_time_str_equals`).
* **Documented Limitation:** SHA-256 is compute-intensive rather than memory-hard (unlike Argon2id or bcrypt). In high-threat environments, dedicated GPU/ASIC clusters can compute SHA-256 rounds faster than memory-hard primitives. For this academic CSE214 project written in pure C99 without third-party cryptographic libraries, 5,000-round salted SHA-256 provides strong self-contained security with zero external dependencies and backward compatibility with existing accounts.
* **Credential Hygiene:** Admin password hashes and salts are never returned in JSON API responses. Passwords are never stored in browser storage. Sessions are managed exclusively via HTTP-only, SameSite=Lax cookies (`bl_session`).

---

## 4. How to Run Locally

1. **Start Server:**
   ```powershell
   .\backend\server.exe 8080
   ```
2. Open **[http://localhost:8080/admin](http://localhost:8080/admin)** in your browser.
3. The database is in a clean initial state: enter your chosen **Admin Email Address** and a secure password. Once configured, you can log in using that email and password.
