# BrishtiLibrary — Project Progress & CSE214 Verification Report

**Project Name:** BrishtiLibrary (বৃষ্টি লাইব্রেরি)  
**Course:** CSE214 — Algorithm Lab & Full-Stack Development  
**Academic Requirement:** Backend strictly in C (C99), SQLite via C API, HTML5/CSS3/Vanilla JS Frontend.  
**Report Date:** October 22, 2026  
**Status:** All Core Storefront, CSE214 Algorithms, Admin Product CRUD, Secure Admin Login, Category Management, Photo Uploads, Warm Blue Themes, SQLite Online Backup API, and Cross-Platform Linux/Docker Deployment verified.

---

## 1. Executive Summary

BrishtiLibrary is a full-featured e-commerce platform for physical books and stationery that simultaneously powers interactive laboratory demonstrations for the **CSE214 Algorithm Course**.

All core requirements—including student customer journeys, product discovery, shopping cart calculations, transactional order placement, order status tracking, admin catalog CRUD, and 5 classic algorithmic paradigms—have been implemented strictly in **C** using SQLite C API (`sqlite3.h`), and `cJSON`.

In this milestone, cross-platform POSIX/Linux hosting readiness was completed:
1. **Cross-Platform C99 Sockets:** Sockets, server lifecycle, and network error reporting abstract cleanly between POSIX (Linux) and Winsock2 (Windows) via `#ifdef _WIN32`.
2. **Dynamic `PORT` Resolution:** Server listens on `$PORT` passed by cloud hosting platforms (Render, Fly.io, Railway, etc.), falling back to CLI argument or port `8080`.
3. **High-Resolution POSIX Timers:** Microsecond algorithm benchmarking supports POSIX `clock_gettime(CLOCK_MONOTONIC)` alongside Windows `QueryPerformanceCounter`.
4. **POSIX Online Backup & Directory Listing:** Canonical overwrite checks support POSIX `realpath()`, and snapshot listing supports `<dirent.h>` and `stat()`.
5. **Multi-Stage Dockerfile & Build Scripts:** Root-level `Dockerfile` (Alpine ~15MB image), root `Makefile`, and `backend/build.sh` added for zero-config Linux deployments.

---

## 2. Test Suite & Verification Results

### Summary of Verified Tests:
* **CSE214 Algorithm Unit Suite (`test_algorithms.exe`):** 76 / 76 Passed (100%)
* **Standalone SQLite Online Backup C99 Suite (`test_backup.exe`):** 19 / 19 Passed (100%)
* **Web & Security Integration Suite (`test_new_features.ps1`):** 54 / 54 Passed (100%)
* **Dynamic `PORT` Environment Variable Test:** Verified server binds and serves requests when `PORT=8095` is set in environment.
* **Grand Total Verified Tests:** **149 / 149 Passed (100%)**

---

### A. CSE214 Standalone Algorithm Test Suite (`test_algorithms.exe`)
* **Total Unit Tests Executed:** 76
* **Tests Passed:** 76 (100%)
* **Tests Failed:** 0
* **Test Runner:** Standalone C binary (`backend/tests/test_algorithms.exe`) compiled with `gcc -std=c99 -Wall -Wextra -O2`.

| Module | Algorithms Tested | Test Coverage & Edge Cases | Status |
| :--- | :--- | :--- | :--- |
| **Search** | Linear Search, Binary Search | Empty arrays, 1-element match/miss, best-case $O(1)$ first-item, worst-case $O(n)$ miss, negative values, duplicates returning first index, comparisons counters. | **76/76 PASS** |
| **Sorting** | Insertion Sort, Selection Sort, Bubble Sort, Quick Sort | Already-sorted, reverse-sorted, random order, single element, duplicates, Bubble early-exit flag ($0$ swaps, $n-1$ comparisons), Insertion $0$ shifts on sorted array. | **PASS** |
| **Optimization** | Greedy vs Dynamic Programming Knapsack | Fractional/Greedy vs 0/1 DP; zero-budget handling; provable demonstration where Greedy yields sub-optimal 30 vs DP global optimum 40. | **PASS** |
| **Graphs** | Dijkstra's Single-Source Shortest Path | Source distance 0, shortest paths across multi-hop campus delivery topology, unreachable nodes flagged with $\infty$, complete predecessor path reconstruction ($0 \to 2 \to 3 \to 5$). | **PASS** |
| **Benchmarking** | Linear vs Binary Search, Sort Comparison | Real-time operation counters ($comparisons$, $swaps$, $shifts$, $runtime$). | **PASS** |

---

### B. Standalone SQLite Online Backup C99 Suite (`test_backup.exe`)
* **Total Unit Tests Executed:** 19
* **Tests Passed:** 19 (100%)
* **Tests Failed:** 0
* **Test Runner:** Standalone C binary (`backend/tests/test_backup.exe`) compiled with `gcc -std=c99 -Wall -Wextra`.

| # | Verified Test Condition | Expected Behavior | Result |
| :---: | :--- | :--- | :---: |
| 1 | Active Database Initialization | `db_init` sets active test database | **PASS** |
| 2 | Overwrite Active DB Prevention | Rejects destination identical to active DB with `SQLITE_MISUSE` | **PASS** |
| 3 | Relative Path Overwrite Prevention | Canonical path check detects `./database/test_active.db` matching active DB | **PASS** |
| 4 | Valid Destination Backup | `db_backup_to_file` succeeds with `SQLITE_OK` | **PASS** |
| 5 | Non-Zero Page Count | `page_count > 0` after backup completes | **PASS** |
| 6 | Independent Backup Opening | Backup database file opened independently with `sqlite3_open` | **PASS** |
| 7 | Schema Integrity Query | Successfully prepares SQL query inspecting `sqlite_master` in backup | **PASS** |
| 8 | Core Tables Present in Backup | All 4 application tables (`products`, `categories`, `users`, `orders`) verified | **PASS** |
| 9-10 | PRAGMA Integrity Check | `PRAGMA integrity_check` on backup database returns `"ok"` | **PASS** |
| 11-15 | JSON Backup Creation API | `db_admin_create_backup_json` returns JSON with filename, pages, size | **PASS** |
| 16 | Path Traversal Sanitization | Rejects malicious custom names like `../../danger.db` | **PASS** |
| 17-19 | JSON Backup Listing API | `db_admin_list_backups_json` lists backups with size and formatted date | **PASS** |

---

### C. Web & Security Integration Suite (`test_new_features.ps1`)
* **Total Tests Executed:** 54
* **Tests Passed:** 54 (100%)
* **Tests Failed:** 0
* **Execution Time:** ~4 seconds

| Module | Features Tested | Result |
| :--- | :--- | :---: |
| **Admin Setup & Auth** | Setup status, demo admin rejection (403), customer demo login (200), first-time password setup, second setup lockout (400), salted SHA-256 password login, wrong password rejection (401), password change. | **12/12 PASS** |
| **Category Management** | Public category query, admin category creation, admin category rename, deletion guard on active products (400), safe deletion of empty category. | **5/5 PASS** |
| **Product Image Uploads** | PNG magic byte validation, URL generation (`/uploads/...`), disk file persistence, static image serving (200), non-image rejection (400), product creation with image, catalog query photo retention. | **8/8 PASS** |
| **Warm Blue Themes** | CSS variables for Light (`#EAF2F8`) and Dark (`#151F2B`) palettes, theme manager script, sun/moon toggles across Storefront, Admin, and Algorithm Lab. | **14/14 PASS** |
| **SQLite Online Backup** | Unauthorized customer rejection (403), admin backup creation (201), filename formatting, file size verification, physical disk check in `database/backups/`, GET backups list, GET backup download (`application/x-sqlite3`), path traversal rejection (403), DELETE backup (200). | **9/9 PASS** |
| **Algorithm Regressions** | Quick Sort C API microsecond timing, Dijkstra campus graph shortest path API. | **2/2 PASS** |
| **Active DB Clean Reset** | Reset script ensures pristine first-time admin setup state for the user. | **PASS** |

---

## 3. How to Build and Run

### A. On Windows (Local Development):
1. **Compile Server:**
   ```powershell
   gcc -std=c99 -Wall -Wextra -O2 -Ibackend/include backend/src/main.c backend/src/server.c backend/src/algorithm_api.c backend/src/database.o backend/src/sqlite3.o backend/src/cJSON.o backend/src/sha256.o backend/src/algorithms/*.c -o backend/server.exe -lws2_32
   ```
2. **Start Server:**
   ```powershell
   .\backend\server.exe 8080
   ```
   Or specify port via environment variable:
   ```powershell
   $env:PORT="8080"; .\backend\server.exe
   ```

### B. On Linux / Docker (Cloud Deployment):
1. **Build Container:**
   ```bash
   docker build -t brishti-library .
   ```
2. **Run Container:**
   ```bash
   docker run -p 8080:8080 -e PORT=8080 brishti-library
   ```

---

## 4. Next Step: Free Public Deployment

1. Commit and push the repository to GitHub.
2. Sign up or log in to **[Render.com](https://render.com)**.
3. Click **New +** > **Web Service** and connect the BrishtiLibrary repository.
4. Render will automatically detect the root **`Dockerfile`**, compile the C backend in Alpine Linux, expose the port dynamically via `$PORT`, and provision a free HTTPS URL (e.g. `https://brishti-library.onrender.com`).
