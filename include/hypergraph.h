#pragma once

#include <stdio.h>

typedef struct
{
    int n, m;
    int *Vd, *Va, *Ed, *Ea;
    int **V, **E;
} hypergraph;

hypergraph *hypergraph_parse(FILE *f);

void hypergraph_free(hypergraph *g);

// Modify

void hypergraph_remove_vertex(hypergraph *g, int u);

void hypergraph_remove_edge(hypergraph *g, int e);

// Utility

void hypergraph_sort(hypergraph *g);

int hypergraph_validate(hypergraph *g);