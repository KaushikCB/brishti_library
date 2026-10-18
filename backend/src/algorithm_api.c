#include "../include/algorithm_api.h"
#include "../include/algorithms.h"
#include "../include/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_int_array(cJSON *arr_json, int *out_arr, int max_size) {
    if (!cJSON_IsArray(arr_json)) return 0;
    int count = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, arr_json) {
        if (count >= max_size) break;
        if (cJSON_IsNumber(item)) {
            out_arr[count++] = item->valueint;
        }
    }
    return count;
}

static LabItem default_lab_items[] = {
    {1, "Monsoon Algorithms", 450, 95},
    {2, "The Quiet Stack", 320, 75},
    {3, "Paper Weather", 280, 60},
    {4, "Graph of Home", 520, 90},
    {5, "Rain Hall Notebook A5", 150, 45},
    {6, "Forest Ink Pen Fine", 180, 50},
    {7, "Goldclip Highlighters 4pk", 120, 40},
    {8, "Desk Storm Sticky Notes", 90, 30}
};
static const int default_lab_items_count = sizeof(default_lab_items) / sizeof(default_lab_items[0]);

/* Helper to parse custom or default lab items */
static int parse_lab_items(cJSON *items_json, LabItem *out_items, int max_items) {
    if (!cJSON_IsArray(items_json) || cJSON_GetArraySize(items_json) == 0) {
        int count = (default_lab_items_count < max_items) ? default_lab_items_count : max_items;
        for (int i = 0; i < count; i++) {
            out_items[i] = default_lab_items[i];
        }
        return count;
    }

    int count = 0;
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, items_json) {
        if (count >= max_items) break;
        cJSON *id = cJSON_GetObjectItem(item, "id");
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *cost = cJSON_GetObjectItem(item, "cost");
        cJSON *val = cJSON_GetObjectItem(item, "utility_value");

        out_items[count].id = id ? id->valueint : (count + 1);
        if (name && name->valuestring) {
            strncpy(out_items[count].name, name->valuestring, MAX_NAME_LEN - 1);
        } else {
            snprintf(out_items[count].name, MAX_NAME_LEN, "Item #%d", count + 1);
        }
        out_items[count].cost = cost ? cost->valueint : 100;
        out_items[count].utility_value = val ? val->valueint : 10;
        count++;
    }
    return count;
}

/* 1. Linear Search Handler */
char *handle_api_linear_search(const char *json_request) {
    cJSON *root = cJSON_Parse(json_request);
    int arr[MAX_ARRAY_SIZE];
    int n = 0;
    int target = 0;

    if (root) {
        cJSON *arr_json = cJSON_GetObjectItem(root, "array");
        cJSON *t_json = cJSON_GetObjectItem(root, "target");
        if (arr_json) n = parse_int_array(arr_json, arr, MAX_ARRAY_SIZE);
        if (t_json) target = t_json->valueint;
        cJSON_Delete(root);
    }

    LinearSearchResult res = algorithm_linear_search(arr, n, target);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "algorithm", "Linear Search");
    cJSON_AddNumberToObject(resp, "target", target);
    cJSON_AddNumberToObject(resp, "found_index", res.found_index);
    cJSON_AddNumberToObject(resp, "comparisons", res.comparisons);
    cJSON_AddNumberToObject(resp, "time_microseconds", res.time_microseconds);

    cJSON *comp = cJSON_CreateObject();
    cJSON_AddStringToObject(comp, "best_case", "O(1)");
    cJSON_AddStringToObject(comp, "average_case", "O(n)");
    cJSON_AddStringToObject(comp, "worst_case", "O(n)");
    cJSON_AddStringToObject(comp, "auxiliary_space", "O(1)");
    cJSON_AddItemToObject(resp, "complexity", comp);

    cJSON *steps_arr = cJSON_CreateArray();
    for (int i = 0; i < res.step_count; i++) {
        cJSON *step_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(step_obj, "index", res.steps[i].index);
        cJSON_AddNumberToObject(step_obj, "value", res.steps[i].value);
        cJSON_AddBoolToObject(step_obj, "matched", res.steps[i].matched);
        cJSON_AddItemToArray(steps_arr, step_obj);
    }
    cJSON_AddItemToObject(resp, "steps", steps_arr);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* 2. Binary Search Handler */
char *handle_api_binary_search(const char *json_request) {
    cJSON *root = cJSON_Parse(json_request);
    int arr[MAX_ARRAY_SIZE];
    int n = 0;
    int target = 0;

    if (root) {
        cJSON *arr_json = cJSON_GetObjectItem(root, "array");
        cJSON *t_json = cJSON_GetObjectItem(root, "target");
        if (arr_json) n = parse_int_array(arr_json, arr, MAX_ARRAY_SIZE);
        if (t_json) target = t_json->valueint;
        cJSON_Delete(root);
    }

    BinarySearchResult res = algorithm_binary_search(arr, n, target);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "algorithm", "Binary Search");
    cJSON_AddNumberToObject(resp, "target", target);
    cJSON_AddNumberToObject(resp, "found_index", res.found_index);
    cJSON_AddNumberToObject(resp, "comparisons", res.comparisons);
    cJSON_AddNumberToObject(resp, "iterations", res.iterations);
    cJSON_AddNumberToObject(resp, "time_microseconds", res.time_microseconds);

    cJSON *comp = cJSON_CreateObject();
    cJSON_AddStringToObject(comp, "best_case", "O(1)");
    cJSON_AddStringToObject(comp, "average_case", "O(log n)");
    cJSON_AddStringToObject(comp, "worst_case", "O(log n)");
    cJSON_AddStringToObject(comp, "auxiliary_space", "O(1)");
    cJSON_AddItemToObject(resp, "complexity", comp);

    cJSON *steps_arr = cJSON_CreateArray();
    for (int i = 0; i < res.step_count; i++) {
        cJSON *step_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(step_obj, "low", res.steps[i].low);
        cJSON_AddNumberToObject(step_obj, "high", res.steps[i].high);
        cJSON_AddNumberToObject(step_obj, "mid", res.steps[i].mid);
        cJSON_AddNumberToObject(step_obj, "mid_value", res.steps[i].mid_value);
        cJSON_AddNumberToObject(step_obj, "comparison_result", res.steps[i].comparison_result);
        cJSON_AddItemToArray(steps_arr, step_obj);
    }
    cJSON_AddItemToObject(resp, "steps", steps_arr);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* 3. Sorting Handler */
char *handle_api_sort(const char *json_request) {
    cJSON *root = cJSON_Parse(json_request);
    char algo[32] = "insertion";
    int arr[MAX_ARRAY_SIZE];
    int n = 0;

    if (root) {
        cJSON *algo_json = cJSON_GetObjectItem(root, "algorithm");
        cJSON *arr_json = cJSON_GetObjectItem(root, "array");
        if (algo_json && algo_json->valuestring) {
            strncpy(algo, algo_json->valuestring, sizeof(algo) - 1);
        }
        if (arr_json) n = parse_int_array(arr_json, arr, MAX_ARRAY_SIZE);
        cJSON_Delete(root);
    }

    SortResult res;
    const char *algo_name = "Insertion Sort";
    const char *best_c = "O(n)";
    const char *avg_c = "O(n²)";
    const char *worst_c = "O(n²)";
    const char *space_c = "O(1) auxiliary";

    if (strcmp(algo, "selection") == 0) {
        res = algorithm_selection_sort(arr, n);
        algo_name = "Selection Sort";
        best_c = "O(n²)";
        avg_c = "O(n²)";
        worst_c = "O(n²)";
        space_c = "O(1) auxiliary";
    } else if (strcmp(algo, "bubble") == 0) {
        res = algorithm_bubble_sort(arr, n);
        algo_name = "Bubble Sort (with Early-Exit)";
        best_c = "O(n)";
        avg_c = "O(n²)";
        worst_c = "O(n²)";
        space_c = "O(1) auxiliary";
    } else if (strcmp(algo, "quick") == 0) {
        res = algorithm_quick_sort(arr, n);
        algo_name = "Quick Sort (Lomuto Partition)";
        best_c = "O(n log n)";
        avg_c = "O(n log n)";
        worst_c = "O(n²)";
        space_c = "O(log n) stack";
    } else {
        res = algorithm_insertion_sort(arr, n);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "algorithm", algo_name);
    cJSON_AddNumberToObject(resp, "comparisons", (double)res.comparisons);
    cJSON_AddNumberToObject(resp, "swaps", (double)res.swaps);
    cJSON_AddNumberToObject(resp, "shifts", (double)res.shifts);
    cJSON_AddNumberToObject(resp, "time_microseconds", res.time_microseconds);
    cJSON_AddNumberToObject(resp, "pass_count", res.pass_count);

    cJSON *comp = cJSON_CreateObject();
    cJSON_AddStringToObject(comp, "best_case", best_c);
    cJSON_AddStringToObject(comp, "average_case", avg_c);
    cJSON_AddStringToObject(comp, "worst_case", worst_c);
    cJSON_AddStringToObject(comp, "space_complexity", space_c);
    cJSON_AddItemToObject(resp, "complexity", comp);

    cJSON *sorted_json = cJSON_CreateIntArray(res.sorted_array, res.size);
    cJSON_AddItemToObject(resp, "sorted_array", sorted_json);

    cJSON *passes_arr = cJSON_CreateArray();
    for (int p = 0; p < res.pass_count; p++) {
        cJSON *pass_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(pass_obj, "pass_number", res.passes[p].pass_number);
        cJSON_AddNumberToObject(pass_obj, "active_1", res.passes[p].active_index_1);
        cJSON_AddNumberToObject(pass_obj, "active_2", res.passes[p].active_index_2);
        cJSON_AddNumberToObject(pass_obj, "comparisons", res.passes[p].comparisons_in_pass);
        cJSON_AddNumberToObject(pass_obj, "swaps", res.passes[p].swaps_in_pass);
        cJSON_AddNumberToObject(pass_obj, "shifts", res.passes[p].shifts_in_pass);
        cJSON_AddItemToObject(pass_obj, "state", cJSON_CreateIntArray(res.passes[p].array_state, res.passes[p].size));
        cJSON_AddItemToArray(passes_arr, pass_obj);
    }
    cJSON_AddItemToObject(resp, "passes", passes_arr);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* 4. Greedy Budget Optimizer Handler */
char *handle_api_greedy(const char *json_request) {
    cJSON *root = cJSON_Parse(json_request);
    int budget = 600;
    LabItem items[32];
    int count = 0;

    if (root) {
        cJSON *b_json = cJSON_GetObjectItem(root, "budget");
        cJSON *i_json = cJSON_GetObjectItem(root, "items");
        if (b_json && b_json->valueint > 0) budget = b_json->valueint;
        count = parse_lab_items(i_json, items, 32);
        cJSON_Delete(root);
    } else {
        count = parse_lab_items(NULL, items, 32);
    }

    GreedyResult res = algorithm_greedy_budget(items, count, budget);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "algorithm", "Greedy Budget Optimizer");
    cJSON_AddNumberToObject(resp, "budget", res.capacity);
    cJSON_AddNumberToObject(resp, "total_cost", res.total_cost);
    cJSON_AddNumberToObject(resp, "remaining_budget", res.capacity - res.total_cost);
    cJSON_AddNumberToObject(resp, "total_value", res.total_value);
    cJSON_AddNumberToObject(resp, "time_microseconds", res.time_microseconds);
    cJSON_AddStringToObject(resp, "strategy_explanation", res.strategy_explanation);

    cJSON *chosen_arr = cJSON_CreateArray();
    for (int i = 0; i < res.selected_count; i++) {
        int idx = res.selected_indices[i];
        cJSON *item_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(item_obj, "id", items[idx].id);
        cJSON_AddStringToObject(item_obj, "name", items[idx].name);
        cJSON_AddNumberToObject(item_obj, "cost", items[idx].cost);
        cJSON_AddNumberToObject(item_obj, "utility_value", items[idx].utility_value);
        cJSON_AddNumberToObject(item_obj, "density", (items[idx].cost > 0) ? ((double)items[idx].utility_value / items[idx].cost) : 0);
        cJSON_AddItemToArray(chosen_arr, item_obj);
    }
    cJSON_AddItemToObject(resp, "selected_items", chosen_arr);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* 5. Dynamic Programming 0/1 Knapsack Handler */
char *handle_api_knapsack(const char *json_request) {
    cJSON *root = cJSON_Parse(json_request);
    int budget = 600;
    LabItem items[32];
    int count = 0;

    if (root) {
        cJSON *b_json = cJSON_GetObjectItem(root, "budget");
        cJSON *i_json = cJSON_GetObjectItem(root, "items");
        if (b_json && b_json->valueint > 0) budget = b_json->valueint;
        count = parse_lab_items(i_json, items, 32);
        cJSON_Delete(root);
    } else {
        count = parse_lab_items(NULL, items, 32);
    }

    KnapsackResult res = algorithm_dp_knapsack(items, count, budget);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "algorithm", "Dynamic Programming (0/1 Knapsack)");
    cJSON_AddNumberToObject(resp, "budget", res.capacity);
    cJSON_AddNumberToObject(resp, "total_cost", res.total_cost);
    cJSON_AddNumberToObject(resp, "remaining_budget", res.capacity - res.total_cost);
    cJSON_AddNumberToObject(resp, "total_value", res.total_value);
    cJSON_AddNumberToObject(resp, "time_microseconds", res.time_microseconds);

    cJSON *comp = cJSON_CreateObject();
    cJSON_AddStringToObject(comp, "time_complexity", "O(n · W)");
    cJSON_AddStringToObject(comp, "space_complexity", "O(n · W) auxiliary table");
    cJSON_AddStringToObject(comp, "optimality", "Guaranteed Global Optimum for indivisible (0/1) items");
    cJSON_AddItemToObject(resp, "complexity", comp);

    cJSON *chosen_arr = cJSON_CreateArray();
    for (int i = 0; i < res.selected_count; i++) {
        int idx = res.selected_indices[i];
        cJSON *item_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(item_obj, "id", items[idx].id);
        cJSON_AddStringToObject(item_obj, "name", items[idx].name);
        cJSON_AddNumberToObject(item_obj, "cost", items[idx].cost);
        cJSON_AddNumberToObject(item_obj, "utility_value", items[idx].utility_value);
        cJSON_AddItemToArray(chosen_arr, item_obj);
    }
    cJSON_AddItemToObject(resp, "selected_items", chosen_arr);

    /* DP Table dimensions for frontend display */
    cJSON *table_info = cJSON_CreateObject();
    cJSON_AddNumberToObject(table_info, "rows", res.item_count + 1);
    cJSON_AddNumberToObject(table_info, "cols", res.capacity + 1);
    cJSON_AddItemToObject(resp, "table_info", table_info);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* 6. Dijkstra Shortest Path Handler */
char *handle_api_dijkstra(const char *json_request) {
    cJSON *root = cJSON_Parse(json_request);
    int source_node = 0;

    if (root) {
        cJSON *src_json = cJSON_GetObjectItem(root, "source_node");
        if (src_json && src_json->valueint >= 0) source_node = src_json->valueint;
        cJSON_Delete(root);
    }

    DeliveryGraph graph = get_sample_delivery_graph();
    if (source_node >= graph.node_count) source_node = 0;

    DijkstraResult res = algorithm_dijkstra(&graph, source_node);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddStringToObject(resp, "algorithm", "Dijkstra Shortest Path");
    cJSON_AddNumberToObject(resp, "source_node", source_node);
    cJSON_AddStringToObject(resp, "source_name", graph.node_names[source_node]);
    cJSON_AddNumberToObject(resp, "time_microseconds", res.time_microseconds);

    cJSON *nodes_arr = cJSON_CreateArray();
    for (int i = 0; i < graph.node_count; i++) {
        cJSON *node_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(node_obj, "id", i);
        cJSON_AddStringToObject(node_obj, "name", graph.node_names[i]);
        cJSON_AddBoolToObject(node_obj, "reachable", res.reachable[i]);
        cJSON_AddNumberToObject(node_obj, "distance_km", (res.distances[i] == INF_DIST) ? -1 : res.distances[i]);

        cJSON *path_arr = cJSON_CreateArray();
        for (int p = 0; p < res.path_lengths[i]; p++) {
            cJSON_AddItemToArray(path_arr, cJSON_CreateNumber(res.reconstructed_paths[i][p]));
        }
        cJSON_AddItemToObject(node_obj, "path_indices", path_arr);
        cJSON_AddItemToArray(nodes_arr, node_obj);
    }
    cJSON_AddItemToObject(resp, "locations", nodes_arr);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

/* 7. Fair Benchmark Handler */
char *handle_api_benchmark(const char *json_request) {
    cJSON *root = cJSON_Parse(json_request);
    int arr[MAX_ARRAY_SIZE];
    int n = 0;
    int target = 37;

    if (root) {
        cJSON *arr_json = cJSON_GetObjectItem(root, "array");
        cJSON *t_json = cJSON_GetObjectItem(root, "target");
        if (arr_json) n = parse_int_array(arr_json, arr, MAX_ARRAY_SIZE);
        if (t_json) target = t_json->valueint;
        cJSON_Delete(root);
    }

    /* If no array provided, generate a deterministic diverse sample of 25 items */
    if (n <= 0) {
        int sample[] = {64, 25, 12, 22, 11, 90, 45, 78, 33, 19, 58, 88, 3, 50, 71, 9, 37, 82, 14, 60, 41, 99, 1, 28, 77};
        n = sizeof(sample) / sizeof(sample[0]);
        for (int i = 0; i < n; i++) arr[i] = sample[i];
    }

    SortComparisonReport sort_rep = algorithm_compare_sorting(arr, n);
    SearchComparisonReport search_rep = algorithm_compare_searching(arr, n, target);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "success");
    cJSON_AddNumberToObject(resp, "array_size", n);
    cJSON_AddNumberToObject(resp, "search_target", target);

    /* Original array */
    cJSON_AddItemToObject(resp, "original_array", cJSON_CreateIntArray(arr, n));

    /* Sorting Table */
    cJSON *sort_arr = cJSON_CreateArray();

    SortBenchmarkEntry *entries[] = {&sort_rep.insertion, &sort_rep.selection, &sort_rep.bubble, &sort_rep.quick};
    for (int i = 0; i < 4; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "name", entries[i]->name);
        cJSON_AddStringToObject(obj, "time_complexity", entries[i]->time_complexity);
        cJSON_AddStringToObject(obj, "space_complexity", entries[i]->space_complexity);
        cJSON_AddNumberToObject(obj, "comparisons", (double)entries[i]->comparisons);
        cJSON_AddNumberToObject(obj, "swaps", (double)entries[i]->swaps);
        cJSON_AddNumberToObject(obj, "shifts", (double)entries[i]->shifts);
        cJSON_AddNumberToObject(obj, "time_microseconds", entries[i]->time_microseconds);
        cJSON_AddBoolToObject(obj, "verified", entries[i]->is_verified);
        cJSON_AddItemToArray(sort_arr, obj);
    }
    cJSON_AddItemToObject(resp, "sorting_comparison", sort_arr);

    /* Searching Table */
    cJSON *search_arr = cJSON_CreateArray();
    SearchBenchmarkEntry *s_entries[] = {&search_rep.linear, &search_rep.binary};
    for (int i = 0; i < 2; i++) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddStringToObject(obj, "name", s_entries[i]->name);
        cJSON_AddStringToObject(obj, "time_complexity", s_entries[i]->time_complexity);
        cJSON_AddStringToObject(obj, "space_complexity", s_entries[i]->space_complexity);
        cJSON_AddNumberToObject(obj, "comparisons", s_entries[i]->comparisons);
        cJSON_AddNumberToObject(obj, "time_microseconds", s_entries[i]->time_microseconds);
        cJSON_AddNumberToObject(obj, "found_index", s_entries[i]->found_index);
        cJSON_AddItemToArray(search_arr, obj);
    }
    cJSON_AddItemToObject(resp, "searching_comparison", search_arr);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}
