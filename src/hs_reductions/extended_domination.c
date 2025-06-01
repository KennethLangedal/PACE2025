#include "hs_reductions/extended_domination.h"

#include <assert.h>
#include <stdlib.h>

int hs_reductions_extended_domination_reduce_graph(hypergraph *g, int ue, int apply_on_edges, hs_change_list *c, int *fast_set, int fs_count)
{
    if (apply_on_edges == 1)
        return 0;

    int u = ue;
    if (g->Vd[u] != 2)
        return 0;

    int e1 = g->V[u][0];
    int e2 = g->V[u][1];

    if (g->Ed[e1] > g->Ed[e2])
    {
        e1 = e2;
        e2 = g->V[u][0];
    }

    for (int j = 0; j < g->Ed[e1]; j++)
        fast_set[g->E[e1][j]] = fs_count;

    for (int j = 0; j < g->Ed[e2]; j++)
        fast_set[g->E[e2][j]] = fs_count;

    // check edges incident to vertices in smaller edge e1
    for (int j = 0; j < g->Ed[e1]; j++)
    {
        int v = g->E[e1][j];
        if (v == u)
            continue;

        for (int k = 0; k < g->Vd[v]; k++)
        {
            // potential edge dominated by e1 \cup e2
            int e = g->V[v][k];
            if (e == e1 || e == e2)
                continue;

            int next = 0;
            for (int l = 0; l < g->Ed[e]; l++)
            {
                // if there is a vertex not in e1 or in e1, there is no domination
                if (fast_set[g->E[e][l]] != fs_count)
                {
                    next = 1;
                    break;
                }
            }

            if (next == 0)
            {
                hs_reducer_queue_up_neighbors_v(g, u, c);
                hypergraph_remove_vertex(g, u);
                return 1;
            }
        }
    }
    return 0;
}