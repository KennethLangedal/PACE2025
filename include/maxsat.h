#pragma once

#include "hypergraph.h"
#include "mwis_reductions.h"

long long maxsat_solve_hitting_set(hypergraph *hg);

int* maxsat_solve_MWIS(graph *g);