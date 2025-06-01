#pragma once

#include "hypergraph.h"
#include "mwis_reductions.h"

graph *hs_reductions_to_mwis(hypergraph *g, int max_degree, long long *offset);
