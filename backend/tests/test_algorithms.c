#include "../include/algorithms.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        g_tests_failed++; \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
    } \
} while(0)

/* Helper for stdlib qsort */
static int int_cmp(const void *a, const void *b) {
    return (*(const int *)a - *(const int *)b);
}

static bool verify_arrays_equal(const int *a, const int *b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/* 1. Test Linear Search */
static void test_linear_search(void) {
    printf("\n=== Testing Linear Search ===\n");

    /* Empty input */
    LinearSearchResult r0 = algorithm_linear_search(NULL, 0, 42);
    TEST_ASSERT(r0.found_index == -1 && r0.comparisons == 0, "Empty array returns -1 and 0 comparisons");

    /* Single element */
    int single[] = {10};
    LinearSearchResult r1 = algorithm_linear_search(single, 1, 10);
    TEST_ASSERT(r1.found_index == 0 && r1.comparisons == 1, "Single element match at index 0");
    LinearSearchResult r1_miss = algorithm_linear_search(single, 1, 99);
    TEST_ASSERT(r1_miss.found_index == -1 && r1_miss.comparisons == 1, "Single element miss returns -1");

    /* Best case (first position) */
    int arr[] = {5, 20, -8, 15, 99, 15, -40};
    int n = sizeof(arr) / sizeof(arr[0]);
    LinearSearchResult r_best = algorithm_linear_search(arr, n, 5);
    TEST_ASSERT(r_best.found_index == 0 && r_best.comparisons == 1, "Best case: target at index 0 takes 1 comparison");

    /* Worst case / missing */
    LinearSearchResult r_miss = algorithm_linear_search(arr, n, 12345);
    TEST_ASSERT(r_miss.found_index == -1 && r_miss.comparisons == n, "Miss case scans all n elements");

    /* Negative number match */
    LinearSearchResult r_neg = algorithm_linear_search(arr, n, -40);
    TEST_ASSERT(r_neg.found_index == 6 && r_neg.comparisons == 7, "Negative number found correctly at last index");

    /* Duplicates: returns first occurrence */
    LinearSearchResult r_dup = algorithm_linear_search(arr, n, 15);
    TEST_ASSERT(r_dup.found_index == 3, "Duplicate target returns first occurrence (index 3)");
}

/* 2. Test Binary Search */
static void test_binary_search(void) {
    printf("\n=== Testing Binary Search ===\n");

    /* Empty input */
    BinarySearchResult b0 = algorithm_binary_search(NULL, 0, 10);
    TEST_ASSERT(b0.found_index == -1 && b0.comparisons == 0, "Empty array returns -1 and 0 comparisons");

    /* Single element */
    int single[] = {25};
    BinarySearchResult b1 = algorithm_binary_search(single, 1, 25);
    TEST_ASSERT(b1.found_index == 0, "Single element match found");
    BinarySearchResult b1_miss = algorithm_binary_search(single, 1, 30);
    TEST_ASSERT(b1_miss.found_index == -1, "Single element miss returns -1");

    /* Boundary cases on sorted array */
    int sorted[] = {-50, -20, -5, 0, 4, 12, 18, 25, 40, 95};
    int n = sizeof(sorted) / sizeof(sorted[0]);

    /* Min boundary */
    BinarySearchResult b_min = algorithm_binary_search(sorted, n, -50);
    TEST_ASSERT(b_min.found_index == 0, "Found min element at index 0");

    /* Max boundary */
    BinarySearchResult b_max = algorithm_binary_search(sorted, n, 95);
    TEST_ASSERT(b_max.found_index == n - 1, "Found max element at index n-1");

    /* Target smaller than min / larger than max */
    BinarySearchResult b_under = algorithm_binary_search(sorted, n, -100);
    TEST_ASSERT(b_under.found_index == -1, "Target smaller than min returns -1");
    BinarySearchResult b_over = algorithm_binary_search(sorted, n, 200);
    TEST_ASSERT(b_over.found_index == -1, "Target larger than max returns -1");

    /* Target inside range but absent */
    BinarySearchResult b_mid_miss = algorithm_binary_search(sorted, n, 10);
    TEST_ASSERT(b_mid_miss.found_index == -1, "Absent target in middle returns -1");

    /* Duplicates: guarantees first occurrence */
    int dups[] = {2, 5, 5, 5, 8, 12, 15};
    int n_dups = sizeof(dups) / sizeof(dups[0]);
    BinarySearchResult b_dup = algorithm_binary_search(dups, n_dups, 5);
    TEST_ASSERT(b_dup.found_index == 1, "Duplicate target 5 returns first occurrence at index 1");
}

/* 3. Test Sorting Algorithms */
static void test_sorting_algorithms(void) {
    printf("\n=== Testing Sorting Algorithms (Insertion, Selection, Bubble, Quick) ===\n");

    int test_cases[][10] = {
        {0},                                        /* Empty (size 0) */
        {42},                                       /* Single element */
        {1, 2, 3, 4, 5, 6, 7},                      /* Already sorted */
        {7, 6, 5, 4, 3, 2, 1},                      /* Reverse sorted */
        {5, 2, 8, 5, 2, 8, 5},                      /* Duplicates */
        {0, -10, 50, -30, 25, -5, 100, -100, 15}    /* Mixed with negatives */
    };
    int test_sizes[] = {0, 1, 7, 7, 7, 9};
    int num_cases = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (int c = 0; c < num_cases; c++) {
        int n = test_sizes[c];
        int original[10];
        int expected[10];
        for (int k = 0; k < n; k++) {
            original[k] = test_cases[c][k];
            expected[k] = test_cases[c][k];
        }
        qsort(expected, n, sizeof(int), int_cmp);

        /* 1. Insertion Sort */
        SortResult res_ins = algorithm_insertion_sort(original, n);
        TEST_ASSERT(is_array_sorted(res_ins.sorted_array, res_ins.size) &&
                    verify_arrays_equal(res_ins.sorted_array, expected, n),
                    "Insertion Sort produces correct sorted output");

        /* 2. Selection Sort */
        SortResult res_sel = algorithm_selection_sort(original, n);
        TEST_ASSERT(is_array_sorted(res_sel.sorted_array, res_sel.size) &&
                    verify_arrays_equal(res_sel.sorted_array, expected, n),
                    "Selection Sort produces correct sorted output");

        /* 3. Bubble Sort */
        SortResult res_bub = algorithm_bubble_sort(original, n);
        TEST_ASSERT(is_array_sorted(res_bub.sorted_array, res_bub.size) &&
                    verify_arrays_equal(res_bub.sorted_array, expected, n),
                    "Bubble Sort produces correct sorted output");

        /* 4. Quick Sort */
        SortResult res_qck = algorithm_quick_sort(original, n);
        TEST_ASSERT(is_array_sorted(res_qck.sorted_array, res_qck.size) &&
                    verify_arrays_equal(res_qck.sorted_array, expected, n),
                    "Quick Sort produces correct sorted output");
    }

    /* Verify Bubble Sort early-exit on already sorted array */
    int already_sorted[] = {10, 20, 30, 40, 50};
    int n_sorted = 5;
    SortResult b_sorted = algorithm_bubble_sort(already_sorted, n_sorted);
    TEST_ASSERT(b_sorted.swaps == 0, "Bubble Sort performs 0 swaps on already sorted array");
    TEST_ASSERT(b_sorted.pass_count == 1, "Bubble Sort early-exit terminates in exactly 1 pass");
    TEST_ASSERT(b_sorted.comparisons == 4, "Bubble Sort performs exactly n-1 (4) comparisons on sorted array");

    /* Verify Insertion Sort best-case on already sorted array */
    SortResult i_sorted = algorithm_insertion_sort(already_sorted, n_sorted);
    TEST_ASSERT(i_sorted.shifts == 0, "Insertion Sort performs 0 shifts on already sorted array");
    TEST_ASSERT(i_sorted.comparisons == 4, "Insertion Sort performs n-1 comparisons on sorted array");
}

/* 4. Test Greedy vs Dynamic Programming */
static void test_greedy_and_dp(void) {
    printf("\n=== Testing Greedy and Dynamic Programming (0/1 Knapsack) ===\n");

    /* Zero capacity */
    LabItem items[] = {
        {1, "Algorithm Book", 500, 80},
        {2, "Gel Pen Set", 150, 40},
        {3, "Dot Grid Notebook", 200, 50},
        {4, "Desk Lamp", 600, 90}
    };
    int count = sizeof(items) / sizeof(items[0]);

    GreedyResult g_zero = algorithm_greedy_budget(items, count, 0);
    TEST_ASSERT(g_zero.selected_count == 0 && g_zero.total_value == 0, "Greedy handles zero budget");

    KnapsackResult dp_zero = algorithm_dp_knapsack(items, count, 0);
    TEST_ASSERT(dp_zero.selected_count == 0 && dp_zero.total_value == 0, "DP Knapsack handles zero budget");

    /* Known textbook counterexample where Greedy is suboptimal for 0/1 items */
    /* Item 1: cost 6, value 30 (density 5.0) */
    /* Item 2: cost 5, value 20 (density 4.0) */
    /* Item 3: cost 5, value 20 (density 4.0) */
    /* Budget: 10 */
    /* Greedy picks Item 1 (cost 6, val 30), remaining budget 4 cannot afford any other item. Total = 30. */
    /* DP picks Item 2 + Item 3 (cost 5+5=10, val 20+20=40). Optimal = 40! */
    LabItem counter_items[] = {
        {101, "Dense Single Book", 6, 30},
        {102, "Textbook A", 5, 20},
        {103, "Textbook B", 5, 20}
    };
    int counter_count = 3;
    int budget = 10;

    GreedyResult g_res = algorithm_greedy_budget(counter_items, counter_count, budget);
    KnapsackResult dp_res = algorithm_dp_knapsack(counter_items, counter_count, budget);

    TEST_ASSERT(g_res.total_value == 30, "Greedy picks locally optimal item 1 (Total value = 30)");
    TEST_ASSERT(dp_res.total_value == 40, "DP finds global optimum (Total value = 40)");
    TEST_ASSERT(dp_res.total_cost == 10, "DP uses full budget (Total cost = 10)");
    TEST_ASSERT(dp_res.selected_count == 2, "DP selects both items 2 and 3");
    TEST_ASSERT(dp_res.total_value > g_res.total_value, "Educational demonstration: DP strictly outperforms Greedy for 0/1 knapsack");
}

/* 5. Test Dijkstra Shortest Path */
static void test_dijkstra(void) {
    printf("\n=== Testing Dijkstra Shortest Path ===\n");

    DeliveryGraph graph = get_sample_delivery_graph();

    /* Run Dijkstra from source = 0 (Main Library Warehouse) */
    DijkstraResult res = algorithm_dijkstra(&graph, 0);

    /* Verify distances */
    TEST_ASSERT(res.distances[0] == 0, "Source node distance to itself is 0");
    TEST_ASSERT(res.distances[1] == 3, "Shortest distance to Rain Hall (1) is 3");
    TEST_ASSERT(res.distances[2] == 2, "Shortest distance to Science Faculty (2) is 2");
    TEST_ASSERT(res.distances[3] == 3, "Shortest distance to Curzon Hall (3) is 3 (via node 2)");
    TEST_ASSERT(res.distances[4] == 7, "Shortest distance to Dhanmondi Hub (4) is 7");
    TEST_ASSERT(res.distances[5] == 11, "Shortest distance to Uttara Drop Point (5) is 11 (0->2->3->5)");

    /* Verify unreachable node 6 (Saint Martin Hub) */
    TEST_ASSERT(res.distances[6] == INF_DIST, "Unreachable node 6 has INF_DIST distance");
    TEST_ASSERT(res.reachable[6] == false, "Unreachable node 6 is flagged reachable=false");

    /* Verify reconstructed path for Node 5 (Uttara) */
    TEST_ASSERT(res.path_lengths[5] == 4, "Path to node 5 consists of 4 vertices");
    int expected_path_5[] = {0, 2, 3, 5};
    bool path_match = true;
    for (int k = 0; k < 4; k++) {
        if (res.reconstructed_paths[5][k] != expected_path_5[k]) {
            path_match = false;
            break;
        }
    }
    TEST_ASSERT(path_match, "Reconstructed path to node 5 is exactly 0 -> 2 -> 3 -> 5");
}

/* 6. Test Algorithm Comparison Runner */
static void test_algorithm_comparisons(void) {
    printf("\n=== Testing Algorithm Comparison Runners ===\n");

    int sample[] = {29, 10, 14, 37, 13, 89, 45, 12, 63, 18};
    int n = sizeof(sample) / sizeof(sample[0]);

    SortComparisonReport s_rep = algorithm_compare_sorting(sample, n);
    TEST_ASSERT(s_rep.array_size == n, "Sorting comparison recorded correct array size");
    TEST_ASSERT(s_rep.insertion.is_verified, "Insertion Sort verified sorted in comparison runner");
    TEST_ASSERT(s_rep.selection.is_verified, "Selection Sort verified sorted in comparison runner");
    TEST_ASSERT(s_rep.bubble.is_verified, "Bubble Sort verified sorted in comparison runner");
    TEST_ASSERT(s_rep.quick.is_verified, "Quick Sort verified sorted in comparison runner");
    TEST_ASSERT(s_rep.insertion.comparisons > 0, "Measured insertion comparisons > 0");
    TEST_ASSERT(s_rep.selection.comparisons == (n * (n - 1) / 2), "Selection comparisons exact n*(n-1)/2");
    TEST_ASSERT(s_rep.bubble.comparisons > 0, "Measured bubble comparisons > 0");
    TEST_ASSERT(s_rep.quick.comparisons > 0, "Measured quick comparisons > 0");

    SearchComparisonReport sr_rep = algorithm_compare_searching(sample, n, 37);
    TEST_ASSERT(sr_rep.array_size == n, "Search comparison recorded correct array size");
    TEST_ASSERT(sr_rep.linear.found_index != -1, "Linear search found target 37");
    TEST_ASSERT(sr_rep.binary.found_index != -1, "Binary search found target 37 in sorted copy");
    TEST_ASSERT(sr_rep.linear.comparisons > 0, "Linear search comparisons counted");
    TEST_ASSERT(sr_rep.binary.comparisons > 0, "Binary search comparisons counted");
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("====================================================\n");
    printf("  CSE214 ALGORITHM LAB - STANDALONE C TEST RUNNER  \n");
    printf("====================================================\n");

    test_linear_search();
    test_binary_search();
    test_sorting_algorithms();
    test_greedy_and_dp();
    test_dijkstra();
    test_algorithm_comparisons();

    printf("\n====================================================\n");
    printf("TEST SUMMARY: Total: %d | Passed: %d | Failed: %d\n",
           g_tests_run, g_tests_passed, g_tests_failed);
    printf("====================================================\n");

    if (g_tests_failed > 0) {
        printf("RESULT: SOME TESTS FAILED!\n");
        return 1;
    }

    printf("RESULT: ALL CSE214 ALGORITHM TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
