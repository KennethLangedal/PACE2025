#pragma once

typedef struct
{
    int n, m, _a;
    int **V, *D, *A, *_A;
    long long *W;
} graph;

graph *graph_init();

void graph_free(graph *g);

// Functions for constructing a graph

void graph_sort_edges(graph *g);

void graph_add_vertex(graph *g, long long w);

void graph_add_edge(graph *g, int u, int v);

// After construction, use these to maintain sorted neighborhoods

void graph_remove_edge(graph *g, int u, int v);

void graph_insert_edge(graph *g, int u, int v);

void graph_deactivate_vertex(graph *g, int u);

void graph_activate_vertex(graph *g, int u);

void graph_deactivate_neighborhood(graph *g, int u);

void graph_activate_neighborhood(graph *g, int u);