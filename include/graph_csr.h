#pragma once

#include "mwis_reductions.h"

typedef struct
{
    int n;
    int *V, *E;
    long long *W;
} graph_csr;

graph_csr *graph_csr_construct(graph *rg, int *FM);

void graph_csr_free(graph_csr *g);

int graph_csr_validate(graph_csr *g);

void graph_csr_subgraph(graph_csr *g, graph_csr *sg, int *Mask, int *RM, int *FM);