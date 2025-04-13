#pragma once

#include "hypergraph.h"
#include "graph.h"

int reduction_vertex_domination(hypergraph *g);

int reduction_edge_domination(hypergraph *g);

graph *reduction_mwis(hypergraph *g);