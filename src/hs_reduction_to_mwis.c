#include "hs_reduction_to_mwis.h"

#include <assert.h>
#include <stdlib.h>

graph *hs_reductions_to_mwis(hypergraph *hg, int max_degree, long long *offset)
{
    graph *g = graph_init();
    *offset = 0;

    long long vw = 1; // (1ll << 32ll);
    long long ew = 2; // (1ll << 33ll);

    for (int i = 0; i < hg->n; i++)
    {
        graph_add_vertex(g, vw);
        *offset += vw;
    }

    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] == 0 || hg->Ed[i] > max_degree)
            continue;
        else if (hg->Ed[i] == 2)
        {
            graph_add_edge(g, hg->E[i][0], hg->E[i][1]);
            continue;
        }
        *offset += ew;
        int s = g->n;
        for (int j = 0; j < hg->Ed[i]; j++)
        {
            graph_add_vertex(g, ew);
        }
        // Make clique
        for (int j = s; j < g->n; j++)
        {
            for (int k = j + 1; k < g->n; k++)
            {
                graph_add_edge(g, j, k);
            }
        }
        // Connect to top layer
        for (int j = 0; j < hg->Ed[i]; j++)
        {
            int v = hg->E[i][j];
            graph_add_edge(g, v, s + j);
        }
    }

    return g;
}
