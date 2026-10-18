#include "../../include/algorithms.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

double get_current_time_microseconds(void) {
    static LARGE_INTEGER frequency;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return ((double)counter.QuadPart * 1000000.0) / (double)frequency.QuadPart;
}

LinearSearchResult algorithm_linear_search(const int *arr, int n, int target) {
    LinearSearchResult res;
    memset(&res, 0, sizeof(res));
    res.target = target;
    res.found_index = -1;
    res.comparisons = 0;
    res.step_count = 0;

    if (n <= 0 || arr == NULL) {
        res.time_microseconds = 0.0;
        return res;
    }

    double start_time = get_current_time_microseconds();

    for (int i = 0; i < n; i++) {
        res.comparisons++;
        
        if (res.step_count < MAX_TRACE_STEPS) {
            res.steps[res.step_count].index = i;
            res.steps[res.step_count].value = arr[i];
            res.steps[res.step_count].matched = (arr[i] == target);
            res.step_count++;
        }

        if (arr[i] == target) {
            res.found_index = i;
            break;
        }
    }

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;
    return res;
}

BinarySearchResult algorithm_binary_search(const int *arr, int n, int target) {
    BinarySearchResult res;
    memset(&res, 0, sizeof(res));
    res.target = target;
    res.found_index = -1;
    res.comparisons = 0;
    res.iterations = 0;
    res.step_count = 0;

    if (n <= 0 || arr == NULL) {
        res.time_microseconds = 0.0;
        return res;
    }

    double start_time = get_current_time_microseconds();

    int low = 0;
    int high = n - 1;
    int candidate_first_occurrence = -1;

    while (low <= high) {
        res.iterations++;
        int mid = low + (high - low) / 2;
        int mid_val = arr[mid];

        BinarySearchStep step;
        step.low = low;
        step.high = high;
        step.mid = mid;
        step.mid_value = mid_val;

        /* Count probe comparison: target vs mid_val */
        res.comparisons++;

        if (mid_val == target) {
            step.comparison_result = 0;
            candidate_first_occurrence = mid;
            /* To ensure deterministic first occurrence on duplicates, continue search in left half */
            high = mid - 1;
        } else if (target < mid_val) {
            step.comparison_result = -1;
            high = mid - 1;
        } else {
            step.comparison_result = 1;
            low = mid + 1;
        }

        if (res.step_count < MAX_TRACE_STEPS) {
            res.steps[res.step_count++] = step;
        }
    }

    res.found_index = candidate_first_occurrence;

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;
    return res;
}

static int search_compare_ints(const void *a, const void *b) {
    return (*(const int *)a - *(const int *)b);
}

SearchComparisonReport algorithm_compare_searching(const int *arr, int n, int target) {
    SearchComparisonReport rep;
    memset(&rep, 0, sizeof(rep));
    rep.array_size = n;

    /* 1. Linear Search on original array */
    LinearSearchResult r_lin = algorithm_linear_search(arr, n, target);
    strncpy(rep.linear.name, "Linear Search", sizeof(rep.linear.name) - 1);
    strncpy(rep.linear.time_complexity, "O(1) best, O(n) worst", sizeof(rep.linear.time_complexity) - 1);
    strncpy(rep.linear.space_complexity, "O(1) auxiliary", sizeof(rep.linear.space_complexity) - 1);
    rep.linear.target = target;
    rep.linear.found_index = r_lin.found_index;
    rep.linear.comparisons = r_lin.comparisons;
    rep.linear.time_microseconds = r_lin.time_microseconds;

    /* 2. Binary Search on sorted copy of array */
    int sorted_copy[MAX_ARRAY_SIZE];
    int copy_size = (n < MAX_ARRAY_SIZE) ? n : MAX_ARRAY_SIZE;
    for (int i = 0; i < copy_size; i++) {
        sorted_copy[i] = arr[i];
    }
    qsort(sorted_copy, copy_size, sizeof(int), search_compare_ints);

    BinarySearchResult r_bin = algorithm_binary_search(sorted_copy, copy_size, target);
    strncpy(rep.binary.name, "Binary Search", sizeof(rep.binary.name) - 1);
    strncpy(rep.binary.time_complexity, "O(1) best, O(log n) worst", sizeof(rep.binary.time_complexity) - 1);
    strncpy(rep.binary.space_complexity, "O(1) iterative", sizeof(rep.binary.space_complexity) - 1);
    rep.binary.target = target;
    rep.binary.found_index = r_bin.found_index;
    rep.binary.comparisons = r_bin.comparisons;
    rep.binary.time_microseconds = r_bin.time_microseconds;

    return rep;
}
