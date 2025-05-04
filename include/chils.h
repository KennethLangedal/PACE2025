#pragma once

#include "local_search.h"

typedef struct
{
    int p;
    double step_time;
    long long step_count;

    long long cost;
    double time;

    local_search **LS, **LS_core;

    graph_csr *d_core;
    int *FM, *RM, *A;
} chils;

chils *chils_init(graph_csr *g, int p, unsigned int seed);

void chils_free(chils *c);

void chils_run(graph_csr *g, chils *c, double tl, long long cl, long long offset, int verbose);

void chils_set_solution(graph_csr *g, chils *c, const int *I);

int *chils_get_best_independent_set(chils *c);