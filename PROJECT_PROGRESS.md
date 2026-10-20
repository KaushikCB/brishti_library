# BrishtiLibrary — Project Progress & CSE214 Verification Report

**Project Name:** BrishtiLibrary (বৃষ্টি লাইব্রেরি)  
**Course:** CSE214 — Algorithm Lab & Full-Stack Development  
**Academic Requirement:** Backend strictly in C (C99), SQLite via C API, HTML5/CSS3/Vanilla JS Frontend.  
**Report Date:** October 21, 2026  
**Status:** All Core Storefront, CSE214 Algorithms, Admin Product CRUD, Secure Admin Login, Category Management, Photo Uploads, and Warm Blue Themes Verified.

---

## 1. Executive Summary

BrishtiLibrary is a full-featured e-commerce platform for physical books and stationery that simultaneously powers interactive laboratory demonstrations for the **CSE214 Algorithm Course**.

All core requirements—including student customer journeys, product discovery, shopping cart calculations, transactional order placement, order status tracking, admin catalog CRUD, and 5 classic algorithmic paradigms—have been implemented strictly in **C** using native Windows Sockets (`winsock2.h`), SQLite C API (`sqlite3.h`), and `cJSON`.

In this latest milestone, four core enhancements were implemented:
1. **Secure Admin Login:** Initial first-time setup for the user to define their own credentials, salted SHA-256 (5,000 rounds) password storage, permanent setup lockout, session verification, and password change.
2. **Category Management:** SQLite backed taxonomy for Books & Stationery with safety guards against deleting categories containing active products.
3. **Product Image Uploads:** C backend Base64 decoder with binary magic byte validation (JPEG, PNG, WebP), safe unique filename generation, persistent storage in `frontend/uploads/`, and fallback illustration when no photo is uploaded.
4. **Warm Blue Light/Dark Themes:** Palette implementation with `#EAF2F8` (Light) and `#151F2B` (Dark), sun/moon instant toggle, `localStorage` persistence, and system preference detection across Storefront, Admin, and Algorithm Lab.

---

## 2. Test Suite & Verification Results

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

### B. New Features & Security Integration Suite (`test_new_features.ps1`)
* **Total Tests Executed:** 41
* **Tests Passed:** 41 (100%)
* **Tests Failed:** 0
* **Execution Time:** ~3.5 seconds

| # | Feature / Test Scenario | Verified Behavior | Result |
| :---: | :--- | :--- | :---: |
| 1 | Setup Status Check | `GET /api/admin/setup-status` returns `setup_completed: false` before setup | **PASS** |
| 2 | Demo Admin Rejection | `POST /api/auth/demo-login` with `role: "admin"` rejected with 403 Forbidden | **PASS** |
| 3 | Customer Demo Login | Customer login creates valid session token and sets `bl_session` cookie | **PASS** |
| 4 | First-Time Admin Setup | Saves salted 5,000-round SHA-256 hash in SQLite users table | **PASS** |
| 5 | Lockout Repeated Setup | Attempting a second setup returns 400 Bad Request ("Setup already completed") | **PASS** |
| 6 | Setup Status Updates | `GET /api/admin/setup-status` returns `setup_completed: true` | **PASS** |
| 7 | Invalid Password Rejected | Login with incorrect password returns 401 Unauthorized | **PASS** |
| 8 | Valid Admin Login | Login with correct password succeeds and returns admin session | **PASS** |
| 9 | Password Change API | Validates current password, updates salt and hash in SQLite | **PASS** |
| 10 | New Password Authentication | New password logs in successfully; old password no longer accepted | **PASS** |
| 11 | Category List API | `GET /api/categories` returns active categories with active product counts | **PASS** |
| 12 | Admin Create Category | Creates new category in SQLite | **PASS** |
| 13 | Admin Update Category | Modifies category name and cascades to existing products | **PASS** |
| 14 | Safe Deletion Protection | Rejecting deletion of category with active products (400 Bad Request) | **PASS** |
| 15 | Admin Delete Empty Category | Deletes empty category cleanly from SQLite | **PASS** |
| 16 | PNG Magic Byte Validation | Accepts valid PNG binary signature (`89 50 4E 47 0D 0A 1A 0A`) | **PASS** |
| 17 | Safe Unique Filename | Generates `bl_<timestamp>_<rand>.png` preventing traversal | **PASS** |
| 18 | Disk File Persistence | Image written to `frontend/uploads/` and preserved across restarts | **PASS** |
| 19 | Static Image Serving | HTTP GET `/uploads/bl_...png` returns 200 with `image/png` header | **PASS** |
| 20 | Reject Disguised Non-Images | Rejects fake base64 text/payload with 400 Bad Request | **PASS** |
| 21 | Product Persistence with Photo | Admin creates product with `image_url` persisted in SQLite | **PASS** |
| 22 | Catalog Photo Retention | Product catalog query returns saved `image_url` | **PASS** |
| 23-27 | Light Mode Palette Integrity | Verifies `#EAF2F8`, `#F2F6FA`, `#6B91C7`, `#263B53`, `#D3DFEA` in CSS | **PASS** |
| 28-32 | Dark Mode Palette Integrity | Verifies `#151F2B`, `#202D3D`, `#789BCB`, `#E5EDF7`, `#35465A` in CSS | **PASS** |
| 33 | Theme Manager Script | `frontend/js/theme.js` exists and binds toggle controls | **PASS** |
| 34-36 | HTML Theme Toggles | Storefront, Admin, and Algorithm Lab contain sun/moon toggle button | **PASS** |
| 37-38 | C Algorithm API Regression | Quick Sort and Dijkstra Graph endpoints return microsecond timings | **PASS** |

---

## 3. How to Build and Run Locally

### 1. Run Algorithm Lab Tests:
```powershell
powershell -ExecutionPolicy Bypass -File backend\build.ps1
```

### 2. Compile Server:
```powershell
gcc -std=c99 -Wall -Wextra -O2 -Ibackend/include backend/src/main.c backend/src/server.c backend/src/algorithm_api.c backend/src/database.o backend/src/sqlite3.o backend/src/cJSON.o backend/src/sha256.o backend/src/algorithms/*.c -o backend/server.exe -lws2_32
```

### 3. Start Server:
```powershell
.\backend\server.exe 8080
```
Open **[http://localhost:8080](http://localhost:8080)** in your browser.

### 4. Administrator Setup:
Open **[http://localhost:8080/admin](http://localhost:8080/admin)** in your browser. The initial setup view will appear, allowing you to choose and set your own administrator username and password. Once saved, the first-time setup is permanently closed.
