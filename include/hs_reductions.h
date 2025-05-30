#pragma once

#include "hypergraph.h"
#include "mwis_reductions.h"

int hs_reductions_degree_one_rule(hypergraph *g);

int hs_reductions_degree_two_rule(hypergraph *g);

int hs_reductions_vertex_domination(hypergraph *g);

int hs_reductions_edge_domination(hypergraph *g);

int hs_reductions_counting_rule(hypergraph *g);

graph *hs_reductions_to_mwis(hypergraph *g, int max_degree, long long *offset);
