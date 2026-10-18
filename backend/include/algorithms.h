#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <stddef.h>
#include <stdbool.h>

#define MAX_ARRAY_SIZE 64
#define MAX_TRACE_STEPS 64
#define MAX_NAME_LEN 64
#define MAX_GRAPH_NODES 32
#define INF_DIST 99999999

/* --- Search Data Structures --- */

typedef struct {
    int index;
    int value;
    bool matched;
} SearchStep;

typedef struct {
    int target;
    int found_index;
    int comparisons;
    double time_microseconds;
    int step_count;
    SearchStep steps[MAX_TRACE_STEPS];
} LinearSearchResult;

typedef struct {
    int low;
    int high;
    int mid;
    int mid_value;
    int comparison_result; /* -1 if target < mid_value, 0 if equal, 1 if target > mid_value */
} BinarySearchStep;

typedef struct {
    int target;
    int found_index;
    int comparisons;
    int iterations;
    double time_microseconds;
    int step_count;
    BinarySearchStep steps[MAX_TRACE_STEPS];
} BinarySearchResult;

/* --- Sorting Data Structures --- */

typedef struct {
    int pass_number;
    int array_state[MAX_ARRAY_SIZE];
    int size;
    int active_index_1;
    int active_index_2;
    int comparisons_in_pass;
    int swaps_in_pass;
    int shifts_in_pass;
} SortPassStep;

typedef struct {
    int sorted_array[MAX_ARRAY_SIZE];
    int size;
    long long comparisons;
    long long swaps;
    long long shifts;
    double time_microseconds;
    int pass_count;
    SortPassStep passes[MAX_TRACE_STEPS];
} SortResult;

/* --- Greedy and Dynamic Programming Data Structures --- */

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    int cost;           /* Weight or price in BDT */
    int utility_value;  /* Priority or educational value */
} LabItem;

typedef struct {
    int capacity;
    int item_count;
    int selected_indices[MAX_ARRAY_SIZE];
    int selected_count;
    int total_cost;
    int total_value;
    double time_microseconds;
    char strategy_explanation[512];
} GreedyResult;

typedef struct {
    int capacity;
    int item_count;
    int selected_indices[MAX_ARRAY_SIZE];
    int selected_count;
    int total_cost;
    int total_value;
    double time_microseconds;
} KnapsackResult;

/* --- Graph Data Structures (Dijkstra) --- */

typedef struct {
    int node_count;
    char node_names[MAX_GRAPH_NODES][MAX_NAME_LEN];
    int adj_matrix[MAX_GRAPH_NODES][MAX_GRAPH_NODES]; /* 0 or INF_DIST means no direct edge */
} DeliveryGraph;

typedef struct {
    int source_node;
    int node_count;
    int distances[MAX_GRAPH_NODES];
    int predecessors[MAX_GRAPH_NODES];
    bool visited[MAX_GRAPH_NODES];
    bool reachable[MAX_GRAPH_NODES];
    double time_microseconds;
    int path_lengths[MAX_GRAPH_NODES];
    int reconstructed_paths[MAX_GRAPH_NODES][MAX_GRAPH_NODES]; /* List of node indices on path */
} DijkstraResult;

/* --- Function Prototypes --- */

/* High-resolution timer helper */
double get_current_time_microseconds(void);

/* Searching */
LinearSearchResult algorithm_linear_search(const int *arr, int n, int target);
BinarySearchResult algorithm_binary_search(const int *arr, int n, int target);

/* Sorting */
SortResult algorithm_insertion_sort(const int *arr, int n);
SortResult algorithm_selection_sort(const int *arr, int n);
SortResult algorithm_bubble_sort(const int *arr, int n);
SortResult algorithm_quick_sort(const int *arr, int n);

/* Helper to verify sortedness */
bool is_array_sorted(const int *arr, int n);

/* Greedy Algorithm (Physical item budget selection: value-per-cost heuristic) */
GreedyResult algorithm_greedy_budget(const LabItem *items, int item_count, int budget);

/* Dynamic Programming (0/1 Knapsack: exact optimum) */
KnapsackResult algorithm_dp_knapsack(const LabItem *items, int item_count, int budget);

/* Graph Algorithm (Dijkstra Shortest Path) */
DijkstraResult algorithm_dijkstra(const DeliveryGraph *graph, int source_node);
DeliveryGraph get_sample_delivery_graph(void);

/* --- Benchmark & Comparison Data Structures --- */

typedef struct {
    char name[32];
    char time_complexity[32];
    char space_complexity[32];
    long long comparisons;
    long long swaps;
    long long shifts;
    double time_microseconds;
    bool is_verified;
} SortBenchmarkEntry;

typedef struct {
    int array_size;
    SortBenchmarkEntry insertion;
    SortBenchmarkEntry selection;
    SortBenchmarkEntry bubble;
    SortBenchmarkEntry quick;
} SortComparisonReport;

typedef struct {
    char name[32];
    char time_complexity[32];
    char space_complexity[32];
    int target;
    int found_index;
    int comparisons;
    double time_microseconds;
} SearchBenchmarkEntry;

typedef struct {
    int array_size;
    SearchBenchmarkEntry linear;
    SearchBenchmarkEntry binary;
} SearchComparisonReport;

/* Benchmark Runners */
SortComparisonReport algorithm_compare_sorting(const int *arr, int n);
SearchComparisonReport algorithm_compare_searching(const int *arr, int n, int target);

#endif /* ALGORITHMS_H */
