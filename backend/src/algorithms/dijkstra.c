#include "../../include/algorithms.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

DeliveryGraph get_sample_delivery_graph(void) {
    DeliveryGraph g;
    memset(&g, 0, sizeof(g));

    g.node_count = 7;
    const char *names[] = {
        "Main Library Warehouse",   /* 0: Source/Center */
        "Rain Hall (Dormitories)",   /* 1 */
        "Science & CSE Faculty",    /* 2 */
        "Curzon Hall Campus",       /* 3 */
        "Dhanmondi Student Hub",    /* 4 */
        "Uttara Drop Point",        /* 5 */
        "Isolated Saint Martin Hub" /* 6: Intentionally unreachable node for testing */
    };

    for (int i = 0; i < g.node_count; i++) {
        strncpy(g.node_names[i], names[i], MAX_NAME_LEN - 1);
    }

    /* Initialize all edges as INF_DIST */
    for (int i = 0; i < g.node_count; i++) {
        for (int j = 0; j < g.node_count; j++) {
            g.adj_matrix[i][j] = (i == j) ? 0 : INF_DIST;
        }
    }

    /* Weighted delivery routes (distance in kilometers) */
    /* 0 (Main Warehouse) connects to 1, 2, 4 */
    g.adj_matrix[0][1] = 3;  g.adj_matrix[1][0] = 3;
    g.adj_matrix[0][2] = 2;  g.adj_matrix[2][0] = 2;
    g.adj_matrix[0][4] = 7;  g.adj_matrix[4][0] = 7;

    /* 1 (Rain Hall) connects to 3 */
    g.adj_matrix[1][3] = 4;  g.adj_matrix[3][1] = 4;

    /* 2 (Science Faculty) connects to 3, 4 */
    g.adj_matrix[2][3] = 1;  g.adj_matrix[3][2] = 1;
    g.adj_matrix[2][4] = 6;  g.adj_matrix[4][2] = 6;

    /* 3 (Curzon Hall) connects to 5 */
    g.adj_matrix[3][5] = 8;  g.adj_matrix[5][3] = 8;

    /* 4 (Dhanmondi) connects to 5 */
    g.adj_matrix[4][5] = 5;  g.adj_matrix[5][4] = 5;

    /* Node 6 is disconnected: no edges to node 6 */

    return g;
}

static int min_distance_node(const int *dist, const bool *visited, int n) {
    int min_val = INF_DIST;
    int min_index = -1;

    for (int v = 0; v < n; v++) {
        if (!visited[v] && dist[v] < min_val) {
            min_val = dist[v];
            min_index = v;
        }
    }
    return min_index;
}

DijkstraResult algorithm_dijkstra(const DeliveryGraph *graph, int source_node) {
    DijkstraResult res;
    memset(&res, 0, sizeof(res));

    if (graph == NULL || source_node < 0 || source_node >= graph->node_count) {
        return res;
    }

    res.source_node = source_node;
    res.node_count = graph->node_count;

    double start_time = get_current_time_microseconds();

    for (int i = 0; i < graph->node_count; i++) {
        res.distances[i] = INF_DIST;
        res.predecessors[i] = -1;
        res.visited[i] = false;
        res.reachable[i] = false;
        res.path_lengths[i] = 0;
    }

    res.distances[source_node] = 0;

    for (int count = 0; count < graph->node_count - 1; count++) {
        int u = min_distance_node(res.distances, res.visited, graph->node_count);
        if (u == -1 || res.distances[u] == INF_DIST) {
            break; /* Remaining vertices are unreachable from source */
        }

        res.visited[u] = true;

        for (int v = 0; v < graph->node_count; v++) {
            int weight = graph->adj_matrix[u][v];
            if (!res.visited[v] && weight > 0 && weight != INF_DIST && res.distances[u] != INF_DIST) {
                if (res.distances[u] + weight < res.distances[v]) {
                    res.distances[v] = res.distances[u] + weight;
                    res.predecessors[v] = u;
                }
            }
        }
    }

    /* Path reconstruction for all nodes */
    for (int target = 0; target < graph->node_count; target++) {
        if (res.distances[target] != INF_DIST) {
            res.reachable[target] = true;

            int temp_path[MAX_GRAPH_NODES];
            int path_len = 0;
            int curr = target;

            while (curr != -1) {
                temp_path[path_len++] = curr;
                if (curr == source_node) break;
                curr = res.predecessors[curr];
            }

            res.path_lengths[target] = path_len;
            /* Reverse into source-to-target order */
            for (int k = 0; k < path_len; k++) {
                res.reconstructed_paths[target][k] = temp_path[path_len - 1 - k];
            }
        } else {
            res.reachable[target] = false;
            res.path_lengths[target] = 0;
        }
    }

    double end_time = get_current_time_microseconds();
    res.time_microseconds = end_time - start_time;

    return res;
}
