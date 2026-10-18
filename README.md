# BrishtiLibrary (বৃষ্টি লাইব্রেরী) 🌧️📚

> **Online Bookstore & Stationery Shop with Academic CSE214 Algorithm Lab**  
> **Backend Architecture:** Pure C (ISO C99/C11) with SQLite 3 C API, Winsock2 HTTP Server, and cJSON  
> **Frontend Architecture:** Modern, accessible HTML5, CSS3, and Vanilla JavaScript  

---

## 1. Project Overview

**BrishtiLibrary** is an online bookstore and stationery shop tailored for university students, readers, and developers in Bangladesh. Named after the quiet monsoon rain (*Brishti*), the platform provides student-friendly books, notebooks, and desk companions.

Beyond e-commerce, BrishtiLibrary fulfills the academic requirements for the **CSE214 Algorithm Lab**:
- Every core search, sort, optimization, and graph algorithm is implemented in **pure C**.
- The web frontend communicates with the C backend through standard JSON REST APIs.
- Execution metrics (comparisons, swaps, shifts, and high-resolution microsecond execution timings) are measured directly in C using Windows performance counters (`QueryPerformanceCounter`) without artificial fabrication or client-side JavaScript substitution.

---

## 2. Technology Stack

| Layer | Technology | Details |
|---|---|---|
| **Backend Language** | C (ISO C99 / C11) | MinGW GCC 6.3.0, modular structure (`.h` / `.c`) |
| **HTTP Server** | Pure C (Winsock2) | Multi-route static file & REST API server |
| **Database** | SQLite 3.36 (C API) | Parameterized queries, WAL mode, transactional ACID checkout |
| **JSON Serialization** | cJSON Amalgamation | Dave Gamble's lightweight ANSI C JSON library |
| **High-Res Timer** | Win32 QPC | Microsecond / nanosecond hardware performance counters |
| **Frontend** | HTML5, CSS3, Vanilla JS | Responsive, Fraunces serif & Source Sans typography |
| **Academic Lab** | CSE214 Engine | 9 algorithm topics with 76 automated test assertions |

---

## 3. Directory Structure

```
Brishti_library/
├── frontend/
│   ├── index.html                 # Storefront homepage (books, stationery, cart, orders)
│   ├── pages/
│   │   ├── algorithms.html        # CSE214 Algorithm Lab interactive dashboard
│   │   └── admin.html             # Store administration & fulfillment dashboard
│   ├── css/
│   │   ├── style.css              # Main design system & responsive layout
│   │   ├── algorithms.css         # Lab visualizer, bars, step player, & graph map
│   │   └── admin.css              # Admin stats, fulfillment tables, & badges
│   └── js/
│       ├── app.js                 # Storefront client driver (cart, checkout, orders, auth)
│       ├── algorithms.js          # Lab driver calling C endpoints with step animation
│       └── admin.js               # Admin driver (order status updates, inventory)
├── backend/
│   ├── include/
│   │   ├── algorithms.h           # Data structures and prototypes for 9 CSE214 algorithms
│   │   ├── algorithm_api.h        # JSON request/response handlers for algorithm APIs
│   │   ├── database.h             # SQLite schema, migrations, cart verification & orders
│   │   ├── server.h               # Server configuration and lifecycle prototypes
│   │   ├── sqlite3.h              # Official SQLite 3.36 C API header
│   │   └── cJSON.h                # cJSON parser and serializer header
│   ├── src/
│   │   ├── main.c                 # Application entry point
│   │   ├── server.c               # Winsock2 HTTP request parser, router, CORS, & sessions
│   │   ├── database.c             # Parameterized SQL queries & transactional order placement
│   │   ├── algorithm_api.c        # JSON serialization for all algorithm endpoints
│   │   ├── algorithms/
│   │   │   ├── search.c           # Linear Search & Binary Search
│   │   │   ├── sort.c             # Insertion, Selection, Bubble, & Quick Sort + Benchmark
│   │   │   ├── greedy.c           # Budget item selection with value-density heuristic
│   │   │   ├── knapsack.c         # Exact 0/1 Knapsack Dynamic Programming
│   │   │   └── dijkstra.c         # Dijkstra Shortest Path with delivery network graph
│   │   ├── sqlite3.c              # Pre-compiled SQLite 3.36 engine
│   │   └── cJSON.c                # cJSON implementation
│   ├── tests/
│   │   ├── test_algorithms.c      # Standalone C test runner (76 test cases)
│   │   └── test_algorithms.exe    # Compiled standalone test binary
│   ├── Makefile                   # GNU Make build script
│   ├── build.ps1                  # Windows PowerShell automated build script
│   └── server.exe                 # Compiled production C server executable
├── database/
│   └── brishti.db                 # Persistent SQLite database (WAL mode)
└── README.md
```

---

## 4. CSE214 Algorithm Lab Implementations

All 9 algorithm topics are implemented in `backend/src/algorithms/`:

### A. Linear Search (`search.c`)
- Iteratively scans array elements.
- Records step-by-step probe chips and element comparisons.
- **Time Complexity:** Best: $O(1)$, Average: $O(n)$, Worst: $O(n)$. Auxiliary Space: $O(1)$.

### B. Binary Search (`search.c`)
- Precondition: Array **must** be sorted.
- Traces search window `[Low, Mid, High]` and records mid comparisons at each step.
- Implements deterministic lower-bound scan on duplicate values (always returns first occurrence).
- **Time Complexity:** Best: $O(1)$, Typical/Worst: $O(\log n)$. Auxiliary Space: $O(1)$.

### C. Insertion Sort (`sort.c`)
- Progressively shifts elements greater than key into position.
- Records pass snapshots, comparison counts, and movement/shift counts.
- **Time Complexity:** Best: $O(n)$ ($0$ shifts on sorted array), Average/Worst: $O(n^2)$. Space: $O(1)$.

### D. Selection Sort (`sort.c`)
- Finds minimum element in remaining unsorted subarray and performs at most $n-1$ swaps.
- **Time Complexity:** Best, Average, Worst: $O(n^2)$ ($n(n-1)/2$ comparisons in all cases). Space: $O(1)$.

### E. Bubble Sort with Early-Exit (`sort.c`)
- Compares adjacent pairs. Features early-exit flag `swapped == false` terminating after pass 1 if array is already sorted.
- **Time Complexity:** Best: $O(n)$ ($n-1$ comparisons, $0$ swaps), Average/Worst: $O(n^2)$. Space: $O(1)$.

### F. Quick Sort (`sort.c`)
- Lomuto partitioning scheme with pivot tracing and recursion bounds.
- **Time Complexity:** Average: $O(n \log n)$, Worst: $O(n^2)$. Space: $O(\log n)$ stack depth.

### G. Greedy Algorithm (`greedy.c`)
- Selects whole physical bookstore items by descending value-density heuristic ($\frac{\text{utility}}{\text{cost}}$) within a student budget.
- Explains why greedy decisions can leave unspendable budget for indivisible items.

### H. Dynamic Programming 0/1 Knapsack (`knapsack.c`)
- Recurrence: $\text{DP}(i, w) = \max(\text{DP}(i-1, w), \text{DP}(i-1, w - c_i) + v_i)$.
- Reconstructs exact chosen items via table backtracking. Guarantees true mathematical optimum.
- Dynamically allocated on the heap to safely handle budgets up to $\text{৳}5000+$.

### I. Graph Algorithms — Dijkstra's Shortest Path (`dijkstra.c`)
- Weighted campus delivery network: Warehouse, Rain Hall, Science Faculty, Curzon Hall, Dhanmondi, Uttara, and disconnected Saint Martin Hub.
- Reconstructs shortest path arrays (e.g. $0 \rightarrow 2 \rightarrow 3 \rightarrow 5$). Unreachable nodes correctly flagged with $\infty$ distance.

### J. Fair Comparison Runner (`sort.c`, `search.c`)
- Runs all 4 sorting algorithms on identical copies of the same array.
- Uses Windows `QueryPerformanceCounter` for microsecond timings.
- Accurately distinguishes element comparisons, swaps, shifts, and execution times.

---

## 5. Build and Run Instructions

### Prerequisites
- **Compiler:** MinGW GCC 6.3+ (or Clang) on Windows.
- **Operating System:** Windows 7 / 8 / 10 / 11.
- **Terminal:** PowerShell or Command Prompt.

### Quick Build & Test (Standalone Test Suite)
To compile and run the 76-case algorithm verification test suite:

```powershell
powershell -ExecutionPolicy Bypass -File backend\build.ps1
```

Or using GCC directly:
```powershell
gcc -std=c99 -Wall -Wextra -O2 -Ibackend/include `
    backend/src/algorithms/search.c `
    backend/src/algorithms/sort.c `
    backend/src/algorithms/greedy.c `
    backend/src/algorithms/knapsack.c `
    backend/src/algorithms/dijkstra.c `
    backend/tests/test_algorithms.c `
    -o backend/tests/test_algorithms.exe

.\backend\tests\test_algorithms.exe
```

Expected output:
```
====================================================
  CSE214 ALGORITHM LAB - STANDALONE C TEST RUNNER  
====================================================
=== Testing Linear Search ===
  [PASS] Empty array returns -1 and 0 comparisons
  ...
TEST SUMMARY: Total: 76 | Passed: 76 | Failed: 0
RESULT: ALL CSE214 ALGORITHM TESTS PASSED SUCCESSFULLY!
```

### Compiling and Starting the C Web Server
```powershell
gcc -std=c99 -Wall -Wextra -O2 -Ibackend/include `
    backend/src/main.c `
    backend/src/server.c `
    backend/src/database.o `
    backend/src/algorithm_api.o `
    backend/src/algorithms/search.c `
    backend/src/algorithms/sort.c `
    backend/src/algorithms/greedy.c `
    backend/src/algorithms/knapsack.c `
    backend/src/algorithms/dijkstra.c `
    backend/src/sqlite3.o `
    backend/src/cJSON.o `
    -o backend/server.exe -lws2_32

.\backend\server.exe 8080
```

Open your browser to:
- **Storefront:** [http://localhost:8080](http://localhost:8080)
- **Algorithm Lab:** [http://localhost:8080/algorithms](http://localhost:8080/algorithms)
- **Admin Dashboard:** [http://localhost:8080/admin](http://localhost:8080/admin)

---

## 6. REST API Endpoints

### Algorithms API
- `POST /api/algorithms/search/linear` — Linear search with step trace.
- `POST /api/algorithms/search/binary` — Binary search with bounds trace.
- `POST /api/algorithms/sort` — Insertion, Selection, Bubble, or Quick sort with pass snapshots.
- `POST /api/algorithms/greedy` — Budget item optimizer with value-density heuristic.
- `POST /api/algorithms/dynamic-programming/knapsack` — 0/1 Knapsack DP optimizer.
- `POST /api/algorithms/graph/shortest-path` — Dijkstra shortest delivery path finder.
- `POST /api/algorithms/benchmark` — Fair comparison benchmark on identical array data.

### Store & Inventory API
- `GET /api/products` — Product catalog with `?q=`, `?category=`, `?sort=`, and `?type=`.
- `GET /api/products/{id}` — Single product details.
- `GET /api/categories` — Distinct book and stationery categories.

### Cart & Orders API
- `POST /api/cart/validate` — Server-side stock check and price validation.
- `POST /api/orders` — Atomic transaction order creation with stock decrement.
- `GET /api/orders` — Customer order history and line items.

### Authentication & Admin API
- `GET /api/auth/me` — Current session user profile.
- `POST /api/auth/demo-login` — Instant role login (`customer` or `admin`).
- `POST /api/auth/google` — Google OAuth ID token verification.
- `POST /api/auth/logout` — Invalidate session cookie.
- `GET /api/admin/orders` — Admin order fulfillment list.
- `PATCH /api/admin/orders/{id}` — Admin order status update (`Pending` $\rightarrow$ `Confirmed`, etc.).

---

## 7. Authentication & Payment Setup

### Google Sign-In Setup
1. Visit [Google Cloud Console](https://console.cloud.google.com/apis/credentials).
2. Create an **OAuth 2.0 Client ID** (Web Application).
3. Set Authorized JavaScript origins to `http://localhost:8080`.
4. In `frontend/index.html`, replace `YOUR_GOOGLE_CLIENT_ID.apps.googleusercontent.com` with your Client ID.
5. *Grading Mode:* Instant one-click demo login buttons are provided in the Auth modal for **Student Demo** and **Brishti Admin** accounts.

### Bangladesh Payment Gateway (bKash / Nagad Sandbox)
- BrishtiLibrary supports **Cash on Delivery (Campus Pickup)** and Sandbox modes for **bKash** and **Nagad**.
- Real merchant transactions require signed merchant agreements from bKash Limited or Bangladesh Post Office.
- For production integration, set environment variables:
  ```powershell
  $env:BKASH_APP_KEY = "your_sandbox_app_key"
  $env:BKASH_APP_SECRET = "your_sandbox_app_secret"
  $env:BKASH_USERNAME = "your_sandbox_username"
  $env:BKASH_PASSWORD = "your_sandbox_password"
  ```
- The backend verifies transaction amounts against SQLite order totals before updating payment status. Browser redirects are never trusted as proof of payment.

---

## 8. Academic Verification Summary

| Test Category | Target Component | Status | Assertions Passed |
|---|---|---|---|
| Search Edge Cases | Empty, single, boundary, negatives, duplicates | PASSED | 16 / 16 |
| Sorting Correctness | Empty, single, sorted, reversed, duplicates, mixed | PASSED | 29 / 29 |
| Optimization Optimum | Zero budget, 0/1 knapsack counterexample proof | PASSED | 7 / 7 |
| Graph Shortest Path | Dijkstra path reconstruction, disconnected nodes | PASSED | 10 / 10 |
| Benchmark Comparators | Identical input verification, operation counts | PASSED | 14 / 14 |
| **Total Test Suite** | Standalone C Test Runner | **100% PASSED** | **76 / 76** |

Developed for CSE214 Algorithm Lab demonstration.
