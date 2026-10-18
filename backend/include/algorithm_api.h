#ifndef ALGORITHM_API_H
#define ALGORITHM_API_H

/* Returns dynamically allocated JSON string (caller must free with free()) */
char *handle_api_linear_search(const char *json_request);
char *handle_api_binary_search(const char *json_request);
char *handle_api_sort(const char *json_request);
char *handle_api_greedy(const char *json_request);
char *handle_api_knapsack(const char *json_request);
char *handle_api_dijkstra(const char *json_request);
char *handle_api_benchmark(const char *json_request);

#endif /* ALGORITHM_API_H */
