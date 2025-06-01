#include "hs_reductions/degree_one.h"

#include <assert.h>
#include <stdlib.h>

int hs_reductions_degree_one_reduce_graph(hypergraph *g, int ue, int apply_on_edges, hs_change_list *c, int *fast_set, int fs_count)
{
    if (apply_on_edges)
    {
        int e = ue;
        if (g->Ed[e] != 1)
            return 0;

        int u = g->E[e][0];
        if (g->Vd[u] > 1)
        {
            hs_reducer_queue_up_neighbors_v(g, u, c);
            hypergraph_include_vertex(g, u);
            return 1;
        }
        return 0;
    }

    int u = ue;
    if (g->Vd[u] != 1)
        return 0;

    int e = g->V[u][0];
    if (g->Ed[e] > 1)
    {
        hypergraph_remove_vertex(g, u);
        hs_reducer_queue_up_neighbors_e(g, e, c);
        return 1;
    }
    return 0;
}