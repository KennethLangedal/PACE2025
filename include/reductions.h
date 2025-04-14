#pragma once

#include "hypergraph.h"
#include "graph.h"

int reduction_vertex_domination(hypergraph *g);

int reduction_edge_domination(hypergraph *g);

graph *reduction_hitting_set_to_mwis(hypergraph *g);

// MWIS reductions

#define N_BUFFERS 3

typedef struct
{
    int _a;

    // Buffers
    int **buffers;

    // Fast sets
    int t;
    int **fast_sets;

    // Changed list
    int n_changed;
    int *changed;

    long long offset;
} reduction_data;

typedef int (*func_reduce)(graph *, int, reduction_data *, void **);
typedef void (*func_restore)(graph *, reduction_data *, void *);
typedef void (*func_lift_solution)(int *, void *);
typedef void (*func_clean)(void *);

typedef struct
{
    func_reduce reduce;
    func_restore restore;
    func_lift_solution lift_solution;
    func_clean clean;
    int global;
} reduction;

reduction_data *reduction_data_init(graph *g);

void reduction_data_increase(reduction_data *rd);

void reduction_data_free(reduction_data *rd);