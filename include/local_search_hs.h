#pragma once

#include "graph_csr.h"

#include <signal.h>

typedef struct
{
    // Solution
    int cost, balance;
    long long age;
    double time, time_ref;
    int *hitting_set;

    // Queue structures
    int queue_count;
    int *queue, *in_queue;
    int *prev_queue, *in_prev_queue;

    // Graph structures
    int max_queue;
    int *score, *cover_count, *one_tight, *tabu, *T;

    // Action log
    int log_count, log_alloc, log_enabled;
    int *log;

    unsigned int seed;
} local_search_hs;

local_search_hs *local_search_hs_init(graph_csr *g, unsigned int seed);

void local_search_hs_free(local_search_hs *ls);

void local_search_hs_reset(graph_csr *g, local_search_hs *ls);

void local_search_hs_add_vertex(graph_csr *g, local_search_hs *ls, int u);

void local_search_hs_remove_vertex(graph_csr *g, local_search_hs *ls, int u);

void local_search_hs_exclude_vertex(graph_csr *g, local_search_hs *ls, int u);

void local_search_hs_greedy(graph_csr *g, local_search_hs *ls);

void local_search_hs_perturbe(graph_csr *g, local_search_hs *ls);

void local_search_hs_explore(graph_csr *g, local_search_hs *ls, double tl, volatile sig_atomic_t *tle,
                             long long il, int offset, int verbose);

void local_search_hs_unwind(graph_csr *g, local_search_hs *ls, int log_t);