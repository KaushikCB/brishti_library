#include "../../include/algorithms.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>

bool is_array_sorted(const int *arr, int n) {
    if (arr == NULL || n <= 1) return true;
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) return false;
    }
    return true;
}

static void copy_array_state(SortPassStep *step, const int *arr, int n) {
    step->size = (n < MAX_ARRAY_SIZE) ? n : MAX_ARRAY_SIZE;
    for (int i = 0; i < step->size; i++) {
        step->array_state[i] = arr[i];
    }
}

/* 1. Insertion Sort */
SortResult algorithm_insertion_sort(const int *arr, int n) {
    SortResult res;
    memset(&res, 0, sizeof(res));
    if (n <= 0 || arr == NULL) return res;

    res.size = (n < MAX_ARRAY_SIZE) ? n : MAX_ARRAY_SIZE;
    for (int k = 0; k < res.size; k++) {
        res.sorted_array[k] = arr[k];
    }

    double start_time = get_current_time_microseconds();

    for (int i = 1; i < res.size; i++) {
        int key = res.sorted_array[i];
        int j = i - 1;
        int pass_comparisons = 0;
        int pass_shifts = 0;

        while (j >= 0) {
            pass_comparisons++;
            res.comparisons++;
            if (res.sorted_array[j] > key) {
                res.sorted_array[j + 1] = res.sorted_array[j];
                pass_shifts++;
                res.shifts++;
                j--;
            } else {
                break;
            }
        }
        res.sorted_array[j + 1] = key;
        if (j + 1 != i) {
            /* Count the insertion key placement as a movement/shift */
            res.shifts++;
            pass_shifts++;
        }

        if (res.pass_count < MAX_TRACE_STEPS) {
            SortPassStep *step = &res.passes[res.pass_count++];
            step->pass_number = i;
            step->active_index_1 = j + 1;
            step->active_index_2 = i;
            step->comparisons_in_pass = pass_comparisons;
            step->swaps_in_pass = 0;
            step->shifts_in_pass = pass_shifts;
            copy_array_state(step, res.sorted_array, res.size);
        }
    }

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;
    return res;
}

/* 2. Selection Sort */
SortResult algorithm_selection_sort(const int *arr, int n) {
    SortResult res;
    memset(&res, 0, sizeof(res));
    if (n <= 0 || arr == NULL) return res;

    res.size = (n < MAX_ARRAY_SIZE) ? n : MAX_ARRAY_SIZE;
    for (int k = 0; k < res.size; k++) {
        res.sorted_array[k] = arr[k];
    }

    double start_time = get_current_time_microseconds();

    for (int i = 0; i < res.size - 1; i++) {
        int min_idx = i;
        int pass_comparisons = 0;
        int pass_swaps = 0;

        for (int j = i + 1; j < res.size; j++) {
            pass_comparisons++;
            res.comparisons++;
            if (res.sorted_array[j] < res.sorted_array[min_idx]) {
                min_idx = j;
            }
        }

        if (min_idx != i) {
            int temp = res.sorted_array[i];
            res.sorted_array[i] = res.sorted_array[min_idx];
            res.sorted_array[min_idx] = temp;
            pass_swaps++;
            res.swaps++;
        }

        if (res.pass_count < MAX_TRACE_STEPS) {
            SortPassStep *step = &res.passes[res.pass_count++];
            step->pass_number = i + 1;
            step->active_index_1 = i;
            step->active_index_2 = min_idx;
            step->comparisons_in_pass = pass_comparisons;
            step->swaps_in_pass = pass_swaps;
            step->shifts_in_pass = 0;
            copy_array_state(step, res.sorted_array, res.size);
        }
    }

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;
    return res;
}

/* 3. Bubble Sort (with Early-Exit Optimization) */
SortResult algorithm_bubble_sort(const int *arr, int n) {
    SortResult res;
    memset(&res, 0, sizeof(res));
    if (n <= 0 || arr == NULL) return res;

    res.size = (n < MAX_ARRAY_SIZE) ? n : MAX_ARRAY_SIZE;
    for (int k = 0; k < res.size; k++) {
        res.sorted_array[k] = arr[k];
    }

    double start_time = get_current_time_microseconds();

    for (int i = 0; i < res.size - 1; i++) {
        bool swapped = false;
        int pass_comparisons = 0;
        int pass_swaps = 0;

        for (int j = 0; j < res.size - 1 - i; j++) {
            pass_comparisons++;
            res.comparisons++;
            if (res.sorted_array[j] > res.sorted_array[j + 1]) {
                int temp = res.sorted_array[j];
                res.sorted_array[j] = res.sorted_array[j + 1];
                res.sorted_array[j + 1] = temp;
                pass_swaps++;
                res.swaps++;
                swapped = true;
            }
        }

        if (res.pass_count < MAX_TRACE_STEPS) {
            SortPassStep *step = &res.passes[res.pass_count++];
            step->pass_number = i + 1;
            step->active_index_1 = res.size - 1 - i;
            step->active_index_2 = -1;
            step->comparisons_in_pass = pass_comparisons;
            step->swaps_in_pass = pass_swaps;
            step->shifts_in_pass = 0;
            copy_array_state(step, res.sorted_array, res.size);
        }

        /* Early-exit optimization: if no swaps in this entire pass, array is sorted */
        if (!swapped) {
            break;
        }
    }

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;
    return res;
}

/* 4. Quick Sort (Lomuto Partition Scheme with Pivot Tracing) */
static int quick_sort_partition(int *arr, int low, int high, SortResult *res) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        res->comparisons++;
        if (arr[j] <= pivot) {
            i++;
            if (i != j) {
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
                res->swaps++;
            }
        }
    }

    int temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;
    if (i + 1 != high) {
        res->swaps++;
    }

    if (res->pass_count < MAX_TRACE_STEPS) {
        SortPassStep *step = &res->passes[res->pass_count++];
        step->pass_number = res->pass_count;
        step->active_index_1 = i + 1; /* pivot position */
        step->active_index_2 = high;
        step->comparisons_in_pass = (high - low);
        step->swaps_in_pass = 1;
        step->shifts_in_pass = 0;
        copy_array_state(step, arr, res->size);
    }

    return i + 1;
}

static void quick_sort_recursive(int *arr, int low, int high, SortResult *res) {
    if (low < high) {
        int pi = quick_sort_partition(arr, low, high, res);
        quick_sort_recursive(arr, low, pi - 1, res);
        quick_sort_recursive(arr, pi + 1, high, res);
    }
}

SortResult algorithm_quick_sort(const int *arr, int n) {
    SortResult res;
    memset(&res, 0, sizeof(res));
    if (n <= 0 || arr == NULL) return res;

    res.size = (n < MAX_ARRAY_SIZE) ? n : MAX_ARRAY_SIZE;
    for (int k = 0; k < res.size; k++) {
        res.sorted_array[k] = arr[k];
    }

    double start_time = get_current_time_microseconds();

    quick_sort_recursive(res.sorted_array, 0, res.size - 1, &res);

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;
    return res;
}

SortComparisonReport algorithm_compare_sorting(const int *arr, int n) {
    SortComparisonReport rep;
    memset(&rep, 0, sizeof(rep));
    rep.array_size = n;

    /* 1. Insertion Sort */
    SortResult r_ins = algorithm_insertion_sort(arr, n);
    strncpy(rep.insertion.name, "Insertion Sort", sizeof(rep.insertion.name) - 1);
    strncpy(rep.insertion.time_complexity, "O(n) best, O(n²) worst", sizeof(rep.insertion.time_complexity) - 1);
    strncpy(rep.insertion.space_complexity, "O(1) auxiliary", sizeof(rep.insertion.space_complexity) - 1);
    rep.insertion.comparisons = r_ins.comparisons;
    rep.insertion.swaps = r_ins.swaps;
    rep.insertion.shifts = r_ins.shifts;
    rep.insertion.time_microseconds = r_ins.time_microseconds;
    rep.insertion.is_verified = is_array_sorted(r_ins.sorted_array, r_ins.size);

    /* 2. Selection Sort */
    SortResult r_sel = algorithm_selection_sort(arr, n);
    strncpy(rep.selection.name, "Selection Sort", sizeof(rep.selection.name) - 1);
    strncpy(rep.selection.time_complexity, "O(n²) all cases", sizeof(rep.selection.time_complexity) - 1);
    strncpy(rep.selection.space_complexity, "O(1) auxiliary", sizeof(rep.selection.space_complexity) - 1);
    rep.selection.comparisons = r_sel.comparisons;
    rep.selection.swaps = r_sel.swaps;
    rep.selection.shifts = r_sel.shifts;
    rep.selection.time_microseconds = r_sel.time_microseconds;
    rep.selection.is_verified = is_array_sorted(r_sel.sorted_array, r_sel.size);

    /* 3. Bubble Sort */
    SortResult r_bub = algorithm_bubble_sort(arr, n);
    strncpy(rep.bubble.name, "Bubble Sort", sizeof(rep.bubble.name) - 1);
    strncpy(rep.bubble.time_complexity, "O(n) best, O(n²) worst", sizeof(rep.bubble.time_complexity) - 1);
    strncpy(rep.bubble.space_complexity, "O(1) auxiliary", sizeof(rep.bubble.space_complexity) - 1);
    rep.bubble.comparisons = r_bub.comparisons;
    rep.bubble.swaps = r_bub.swaps;
    rep.bubble.shifts = r_bub.shifts;
    rep.bubble.time_microseconds = r_bub.time_microseconds;
    rep.bubble.is_verified = is_array_sorted(r_bub.sorted_array, r_bub.size);

    /* 4. Quick Sort */
    SortResult r_qck = algorithm_quick_sort(arr, n);
    strncpy(rep.quick.name, "Quick Sort", sizeof(rep.quick.name) - 1);
    strncpy(rep.quick.time_complexity, "O(n log n) avg, O(n²) worst", sizeof(rep.quick.time_complexity) - 1);
    strncpy(rep.quick.space_complexity, "O(log n) stack", sizeof(rep.quick.space_complexity) - 1);
    rep.quick.comparisons = r_qck.comparisons;
    rep.quick.swaps = r_qck.swaps;
    rep.quick.shifts = r_qck.shifts;
    rep.quick.time_microseconds = r_qck.time_microseconds;
    rep.quick.is_verified = is_array_sorted(r_qck.sorted_array, r_qck.size);

    return rep;
}
