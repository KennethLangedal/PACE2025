#pragma once

#include "graph_csr.h"

#include <signal.h>

typedef struct
{
    // Solution
    int cost, best_cost;
    double time, time_ref;
    int *hitting_set, *best_hitting_set;

    // Graph structures
    int *score, *cover_count, *one_tight;

    // SA
    long long k;

    unsigned int seed;
} simulated_annealing;

simulated_annealing *simulated_annealing_init(graph_csr *g, unsigned int seed);

void simulated_annealing_free(simulated_annealing *sa);

void simulated_annealing_reset(graph_csr *g, simulated_annealing *sa);

void simulated_annealing_add_vertex(graph_csr *g, simulated_annealing *sa, int u);

void simulated_annealing_remove_vertex(graph_csr *g, simulated_annealing *sa, int u);

void simulated_annealing_start(graph_csr *g, simulated_annealing *sa, double tl, volatile sig_atomic_t *tle, int offset, int verbose);