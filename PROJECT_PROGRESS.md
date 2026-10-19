# BrishtiLibrary — Project Progress & CSE214 Verification Report

**Project Name:** BrishtiLibrary (বৃষ্টি লাইব্রেরি)  
**Course:** CSE214 — Algorithm Lab & Full-Stack Development  
**Academic Requirement:** Backend strictly in C (C99), SQLite via C API, HTML5/CSS3/Vanilla JS Frontend.  
**Report Date:** October 20, 2026  
**Status:** All Core Storefront, CSE214 Algorithms, Admin Product Management, and E2E Integration Verified.

---

## 1. Executive Summary

BrishtiLibrary is a full-featured e-commerce platform for physical books and stationery that simultaneously powers interactive laboratory demonstrations for the **CSE214 Algorithm Course**.

All core requirements—including student customer journeys, product discovery, shopping cart calculations, transactional order placement, order status tracking, admin catalog CRUD, and 5 classic algorithmic paradigms—have been implemented strictly in **C** using the native Windows Sockets (`winsock2.h`), SQLite C API (`sqlite3.h`), and `cJSON`.

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

### B. End-to-End System Integration Suite (`test_full_suite.ps1`)
* **Total Integration Tests:** 27
* **Tests Passed:** 27 (100%)
* **Tests Failed:** 0
* **Execution Time:** ~3.2 seconds

| # | Test Scenario | Verified Behavior | Result |
| :---: | :--- | :--- | :---: |
| 1 | Storefront Static HTML Serving | Serves homepage with Fraunces/Plus Jakarta design and catalog controls | **PASS** |
| 2 | Categories API (`GET /api/categories`) | Returns all product categories from SQLite | **PASS** |
| 3 | Active Catalog (`GET /api/products`) | Retrieves all active books and stationery products | **PASS** |
| 4 | Search by Title (`?q=Monsoon`) | Matches title substring | **PASS** |
| 5 | Search by Author (`?q=Rahman`) | Matches book author | **PASS** |
| 6 | Search by Brand (`?q=Rain Stationery`) | Matches stationery brand | **PASS** |
| 7 | Search by ISBN (`?q=978-984...`) | Matches exact book SKU/ISBN | **PASS** |
| 8 | Category & Type Filter | Filters by Category ('Computer Science') and Type ('book') | **PASS** |
| 9 | Stock Availability Filter | `availability=in_stock` cleanly excludes out-of-stock items | **PASS** |
| 10 | Price Range Filter | `min_price=200&max_price=700` returns items strictly within boundary | **PASS** |
| 11 | Sort by Price Ascending | Verifies non-decreasing prices ($P_i \le P_{i+1}$) | **PASS** |
| 12 | Sort by Price Descending | Verifies non-increasing prices ($P_i \ge P_{i+1}$) | **PASS** |
| 13 | Product Details API (`GET /api/products/:id`) | Returns complete product specifications | **PASS** |
| 14 | Server-Side Cart Validation | Re-verifies client item prices against DB before order creation | **PASS** |
| 15 | Customer Demo Login | Creates session token and sets `bl_session` cookie | **PASS** |
| 16 | Customer Auth Verification (`/api/auth/me`) | Confirms authenticated customer identity | **PASS** |
| 17 | Atomic Checkout (`POST /api/orders`) | Generates unique order number (`BL-YYYYMMDD-XXXX`) | **PASS** |
| 18 | Atomic Stock Decrement | SQLite transaction decrements stock; rolls back on failure | **PASS** |
| 19 | Customer Order History | User orders query reflects newly placed order with items | **PASS** |
| 20 | Admin Route Security (403 Forbidden) | Customer session blocked from `/api/admin/*` endpoints | **PASS** |
| 21 | Admin Demo Login | Creates session token with `admin` role | **PASS** |
| 22 | Admin Order List (`GET /api/admin/orders`) | Admin retrieves all orders across all customers | **PASS** |
| 23 | Order Fulfillment Update | Admin updates order status (e.g. `Pending` $\to$ `Shipped`) | **PASS** |
| 24 | Real-Time Customer Order Tracking | Customer view dynamically reflects `Shipped` status and stepper | **PASS** |
| 25 | Admin Add Product (`POST /api/admin/products`) | Creates new book/stationery with validated fields and unique SKU | **PASS** |
| 26 | Admin Inline Stock Update (`PATCH /api/admin/products/:id`) | Modifies inventory in real-time | **PASS** |
| 27 | Admin Product Deactivation (`DELETE /api/admin/products/:id`) | Safely deactivates product (`is_active = 0`) preserving foreign keys | **PASS** |

---

## 3. Root Cause Diagnosis & Resolution for `test_full_suite.ps1`

### Root Causes Diagnosed:
1. **Missing Process Working Directory:**
   The test script previously launched `server.exe` using `Start-Process` without specifying `-WorkingDirectory "e:\New folder\project\Brishti_library"`. In `backend/src/main.c`, relative paths are configured for `./frontend` and `./database/brishti.db`. Without explicit working directory specification, child processes inherited caller working paths, failing to load database tables or static assets.
2. **PowerShell 5.1 Internet Explorer COM Dependency:**
   In Windows PowerShell 5.1, `Invoke-WebRequest` defaults to invoking the Internet Explorer MSHTML rendering engine if `-UseBasicParsing` is omitted. If Internet Explorer has not completed its first-run wizard or runs in a headless agent environment, `Invoke-WebRequest` hangs waiting for user interaction or DOM completion.
3. **PowerShell Single-Object Pipeline Gotcha:**
   When filtering orders using `Where-Object`, a single matching item returned a scalar `PSCustomObject` rather than an array. In PowerShell 5.1, `(scalar_object).Count` evaluates to `$null`, causing `$hasPlacedOrder` assertion to fail.

### Fixes Applied:
* Explicitly passed `-WorkingDirectory "e:\New folder\project\Brishti_library"` to `Start-Process`.
* Added `-UseBasicParsing` and `-TimeoutSec 10` across all HTTP cmdlets.
* Wrapped pipeline outputs in `@(...)` array sub-expressions to guarantee `.Count` resolution.

---

## 4. System Architecture & Implementation Details

```
Brishti_library/
├── backend/
│   ├── include/
│   │   ├── algorithms.h      # Search, Sort, Greedy, Knapsack, Dijkstra interfaces
│   │   ├── cJSON.h           # High-performance C JSON parser & serializer
│   │   ├── database.h        # SQLite CRUD, schema migrations, and transactions
│   │   ├── server.h          # HTTP socket server, router, and session parser
│   │   └── sqlite3.h         # SQLite 3 C interface
│   ├── src/
│   │   ├── algorithms/       # C implementation of CSE214 algorithms
│   │   │   ├── dijkstra.c    # Shortest path delivery router
│   │   │   ├── greedy.c      # Greedy coin/item budgeting
│   │   │   ├── knapsack.c    # 0/1 Dynamic Programming knapsack
│   │   │   ├── search.c      # Linear & Binary search with step counters
│   │   │   └── sort.c        # Insertion, Selection, Bubble, Quick sort
│   │   ├── algorithm_api.c   # JSON HTTP wrappers & benchmark runners
│   │   ├── cJSON.c           # cJSON library implementation
│   │   ├── database.c        # Database schema, seed data, transactions, CRUD
│   │   ├── main.c            # Entry point and config initialization
│   │   ├── server.c          # WinSock2 multi-threaded HTTP server & routing
│   │   └── sqlite3.c         # SQLite 3 amalgamation
│   ├── tests/
│   │   ├── test_algorithms.c # Standalone C runner for algorithm validation
│   │   └── test_algorithms.exe
│   ├── build.ps1             # Algorithm build & test runner
│   ├── Makefile              # GCC build script
│   └── server.exe            # Compiled native C web server binary
├── database/
│   └── brishti.db            # Persistent SQLite database (WAL mode enabled)
└── frontend/
    ├── css/
    │   ├── admin.css         # Admin portal styling & modal layouts
    │   └── style.css         # Monsoon burgundy/cream storefront styling
    ├── js/
    │   ├── admin.js          # Admin product CRUD & order management
    │   └── app.js            # Storefront catalog, cart, checkout, tracker
    ├── pages/
    │   └── admin.html        # Admin backoffice dashboard
    └── index.html            # Public storefront homepage
```

---

## 5. Database Schema & Transaction Safety

The database uses SQLite in Write-Ahead Logging (`WAL`) mode with foreign key constraints enabled:
* `PRAGMA journal_mode=WAL;`
* `PRAGMA foreign_keys=ON;`

### Performance Indexes Added:
* `idx_products_type_cat` on `products(type, category)`
* `idx_products_active` on `products(is_active)`
* `idx_orders_user` on `orders(user_id)`
* `idx_sessions_token` on `sessions(session_token)`
* `idx_order_items_order` on `order_items(order_id)`

### Transactional Guarantees:
Order placement uses explicit SQLite transactions (`BEGIN TRANSACTION`, `COMMIT`, `ROLLBACK`):
1. Verifies product existence and available inventory.
2. Inserts parent record into `orders`.
3. Inserts each item into `order_items`.
4. Decrements `products.stock_quantity`.
5. If any item has insufficient stock, the transaction immediately rolls back (`ROLLBACK`) and returns a descriptive error message without partial mutations.

---

## 6. How to Build and Run

### Run Algorithm Lab Tests:
```powershell
powershell -ExecutionPolicy Bypass -File backend\build.ps1
```

### Compile Server:
```powershell
gcc -std=c99 -Wall -Wextra -O2 -Ibackend/include backend/src/main.c backend/src/server.c backend/src/algorithm_api.c backend/src/database.o backend/src/sqlite3.o backend/src/cJSON.o backend/src/algorithms/*.c -o backend/server.exe -lws2_32
```

### Start Server:
```powershell
cd "e:\New folder\project\Brishti_library"
.\backend\server.exe 8080
```
Open [http://localhost:8080](http://localhost:8080) in your browser.

---

## 7. Current Limitations & Academic Evaluation Notes

1. **Third-Party Payment Gateways:**
   Per student lab project rules and security safety, real payment gateways (e.g. bKash / SSLCommerz / Stripe) are simulated using Cash on Delivery and demo transaction tokens to avoid requiring real credentials or costs.
2. **Image Hosting:**
   Products use custom pure-CSS gradient book and stationery covers (`cover-book-1`, `cover-stat-1`, etc.) matching the design system, avoiding external broken image link dependencies.
3. **Session Persistence:**
   Sessions use SQLite backed tokens with HTTP-only cookies (`bl_session`).
