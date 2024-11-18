#include "dom_lb.h"

#include <stdlib.h>
#include <math.h>

int dom_sum_degree_bound(graph *g)
{
    double lb = 0.0;
    for (int u = 0; u < g->n; u++)
    {
        int md = (g->V[u + 1] - g->V[u]) + 1;
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int v = g->E[i];
            int d = (g->V[v + 1] - g->V[v]) + 1;
            if (d > md)
                md = d;
        }

        lb += 1.0 / (double)md;
    }

    return (int)ceil(lb);
}

int dom_efficiency_bound(graph *g)
{
    return 0;
}