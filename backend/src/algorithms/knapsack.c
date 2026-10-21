#include "../../include/algorithms.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

KnapsackResult algorithm_dp_knapsack(const LabItem *items, int item_count, int budget) {
    KnapsackResult res;
    memset(&res, 0, sizeof(res));
    res.capacity = budget;
    res.item_count = item_count;
    res.selected_count = 0;
    res.total_cost = 0;
    res.total_value = 0;

    if (items == NULL || item_count <= 0 || budget <= 0) {
        return res;
    }

    int n = item_count;
    int W = budget;

    /* Allocate DP table on the heap to safely support budgets up to 5000+ BDT */
    int cols = W + 1;
    int *dp = (int *)calloc((size_t)(n + 1) * cols, sizeof(int));
    if (!dp) {
        return res;
    }

    #define DP(i, w) dp[(i) * cols + (w)]

    double start_time = get_current_time_microseconds();

    /* DP Table computation: 
       DP(i, w) = max utility from first i items within budget w */
    for (int i = 0; i <= n; i++) {
        for (int w = 0; w <= W; w++) {
            if (i == 0 || w == 0) {
                DP(i, w) = 0;
            } else if (items[i - 1].cost <= w) {
                int without_item = DP(i - 1, w);
                int with_item = items[i - 1].utility_value + DP(i - 1, w - items[i - 1].cost);
                DP(i, w) = (with_item > without_item) ? with_item : without_item;
            } else {
                DP(i, w) = DP(i - 1, w);
            }
        }
    }

    res.total_value = DP(n, W);

    /* Backtracking to reconstruct exact chosen items */
    int current_w = W;
    int temp_selected[MAX_ARRAY_SIZE];
    int temp_count = 0;

    for (int i = n; i > 0 && current_w > 0; i--) {
        if (DP(i, current_w) != DP(i - 1, current_w)) {
            temp_selected[temp_count++] = i - 1;
            res.total_cost += items[i - 1].cost;
            current_w -= items[i - 1].cost;
        }
    }

    /* Reverse to preserve original catalog order */
    for (int i = 0; i < temp_count; i++) {
        res.selected_indices[i] = temp_selected[temp_count - 1 - i];
    }
    res.selected_count = temp_count;

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;

    free(dp);
    return res;
}
