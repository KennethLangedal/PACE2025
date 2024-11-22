#pragma once

#include "graph.h"

typedef struct
{
    // Solution
    int cost;
    int *dominating_set;

    // Queue structures
    int max_queue, queue_count;
    int *queue, *in_queue;
    int *prev_queue, *in_prev_queue;

    // Graph structures
    int *tightness;

    // Action log
    int log_count, log_enabled;
    int *log;

    // Timing
    double time, time_ref;
    unsigned int seed;
} local_search;

// Init/free

local_search *local_search_init(graph *g, unsigned int seed);

void local_search_free(local_search *ls);

// Run local search for tl seconds

void local_search_explore(graph *g, local_search *ls, double tl, int verbose);

// Helper functions

void local_search_add_vertex(graph *g, local_search *ls, int u);

void local_search_remove_vertex(graph *g, local_search *ls, int u);

void local_search_greedy(graph *g, local_search *ls);

void local_search_perturbe(graph *g, local_search *ls);

void local_search_unwind(graph *g, local_search *ls, int t);

int local_search_validate_solution(graph *g, local_search *ls);