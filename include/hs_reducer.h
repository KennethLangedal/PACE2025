#pragma once

#include "hypergraph.h"

#include <stdarg.h>

typedef struct
{
    long long _a;

    int n;     // Number of vertices changed
    int *V;    // The changed vertices
    int *in_V; // Set to 1 if vertex is in the list
    int m;     // Number of edges changed
    int *E;    // The changed edges
    int *in_E; // Set to 1 if edge is in the list
} hs_change_list;

/*
    Try the reduction from vertex u
    if the reduction is successful, return 1, otherwise 0
    To test the reductions, the fast_set can be used to avoid local allocations. Note that the fast_sets assume
    that no value larger than fs_count is written anywhere, so DO NOT write larger values into these arrays.

    In the case of a successful reduction, the graph should be updated accordingly
    Additionally:
        - the c->n should be set to the number of vertices that observed a change in their neighborhood
        - the c->V should contain these vertices
        - the c->m should be set to the number of edges that observed a change in their neighborhood
        - the c->E should contain these edges
*/
typedef int (*hs_func_reduce_graph)(hypergraph *hg, int u, int apply_on_edges, hs_change_list *c, int *fast_set, int fs_count);

typedef struct
{
    hs_func_reduce_graph reduce;
    int global;
} hs_reduction;

typedef struct
{
    int n_rules;
    hs_reduction *Rule;

    int *Queue_count, *Queue_front, *Queue_back;
    int **Queues;
    int *Queue_count_E, *Queue_front_E, *Queue_back_E;
    int **Queues_E;
    int **In_queues, **In_queues_E;

    int *fast_set;
    int fs_count;
    hs_change_list *c;

    int verbose;
} hs_reducer;

hs_reducer *hs_reducer_init(hypergraph *g, int n_rules, ...);

void hs_reducer_free(hs_reducer *r);

// void hs_reducer_reduce(hs_reducer *r, hypergraph *g, double tl);
void hs_reducer_reduce(hs_reducer *r, hypergraph *g);

void hs_reducer_queue_changed(hypergraph *g, hs_reducer *r);

int hs_reducer_apply_reduction(hypergraph *g, int u, int apply_on_edges, hs_reduction rule, hs_reducer *r);

void hs_reducer_lift_solution(hypergraph *g, int *HS);

void reducer_queue_all(hs_reducer *r, hypergraph *g);

void hs_reducer_queue_up_neighbors_v(hypergraph *g, int u, hs_change_list *c);
void hs_reducer_queue_up_neighbors_e(hypergraph *g, int e, hs_change_list *c);

void hs_reducer_reset_fast_set(hypergraph *g, hs_reducer *r);
