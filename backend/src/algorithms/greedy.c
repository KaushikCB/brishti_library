#include "../../include/algorithms.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    int original_index;
    LabItem item;
    double density; /* utility_value / cost */
} GreedyItemWrapper;

static int compare_density_desc(const void *a, const void *b) {
    const GreedyItemWrapper *ia = (const GreedyItemWrapper *)a;
    const GreedyItemWrapper *ib = (const GreedyItemWrapper *)b;
    if (ia->density < ib->density) return 1;
    if (ia->density > ib->density) return -1;
    /* Secondary tie-breaker: lower cost first */
    return ia->item.cost - ib->item.cost;
}

GreedyResult algorithm_greedy_budget(const LabItem *items, int item_count, int budget) {
    GreedyResult res;
    memset(&res, 0, sizeof(res));
    res.capacity = budget;
    res.item_count = item_count;
    res.selected_count = 0;
    res.total_cost = 0;
    res.total_value = 0;

    if (items == NULL || item_count <= 0 || budget <= 0) {
        snprintf(res.strategy_explanation, sizeof(res.strategy_explanation),
                 "No items or zero budget provided. Greedy selected 0 items.");
        return res;
    }

    double start_time = get_current_time_microseconds();

    GreedyItemWrapper *wrappers = (GreedyItemWrapper *)malloc(item_count * sizeof(GreedyItemWrapper));
    if (!wrappers) {
        res.time_microseconds = 0;
        return res;
    }

    for (int i = 0; i < item_count; i++) {
        wrappers[i].original_index = i;
        wrappers[i].item = items[i];
        wrappers[i].density = (items[i].cost > 0) 
            ? ((double)items[i].utility_value / (double)items[i].cost) 
            : 0.0;
    }

    /* Sort items descending by value density (utility / BDT) */
    qsort(wrappers, item_count, sizeof(GreedyItemWrapper), compare_density_desc);

    int remaining_budget = budget;
    for (int i = 0; i < item_count; i++) {
        if (wrappers[i].item.cost <= remaining_budget) {
            res.selected_indices[res.selected_count++] = wrappers[i].original_index;
            res.total_cost += wrappers[i].item.cost;
            res.total_value += wrappers[i].item.utility_value;
            remaining_budget -= wrappers[i].item.cost;
        }
    }

    free(wrappers);

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;

    snprintf(res.strategy_explanation, sizeof(res.strategy_explanation),
             "Greedy Strategy: Value-per-cost density heuristic (utility/BDT). "
             "Selected %d item(s) totaling ৳%d with score %d. "
             "Note: For 0/1 indivisible physical items, Greedy may make locally optimal "
             "choices that leave unspendable budget, differing from global DP optimum.",
             res.selected_count, res.total_cost, res.total_value);

    return res;
}
