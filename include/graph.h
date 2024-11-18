#pragma once

#include <stdio.h>

typedef struct
{
    int n, m;
    int *V, *E;
} graph;

graph *graph_parse(FILE *f);

void graph_sort_edges(graph *g);

void graph_free(graph *g);

int graph_validate(graph *g);