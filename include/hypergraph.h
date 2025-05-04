#pragma once

#include <stdio.h>

typedef struct
{
    int n, m;
    int *Vd, *Va, *Ed, *Ea;
    int **V, **E;
} hypergraph;

hypergraph *hypergraph_parse(FILE *f);

hypergraph *hypergraph_copy(hypergraph *g);

void hypergraph_free(hypergraph *g);

// Modify

void hypergraph_remove_vertex(hypergraph *g, int u);

void hypergraph_remove_edge(hypergraph *g, int e);

void hypergraph_include_vertex(hypergraph *g, int u);

// Utility

void hypergraph_sort(hypergraph *g);

int hypergraph_validate(hypergraph *g);