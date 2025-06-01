#pragma once

#include "hypergraph.h"
#include "mwis_reductions.h"
#include "hs_reducer.h"

int hs_reductions_counting_rule_reduce_graph(hypergraph *g, int u, int apply_on_edges, hs_change_list *c, int *fast_set, int fs_count);

static hs_reduction counting_rule = {
    .reduce = hs_reductions_counting_rule_reduce_graph,
    .global = 0,
};
