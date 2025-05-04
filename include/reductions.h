#pragma once

#include "hypergraph.h"
#include "graph.h"

int reduction_degree_one_rule(hypergraph *g);

int reduction_vertex_domination(hypergraph *g);

int reduction_edge_domination(hypergraph *g);

int reduction_counting_rule(hypergraph *g);

int reduction_large_edge_rule(hypergraph *g);

graph *reduction_hitting_set_to_mwis(hypergraph *g, long long *offset);
