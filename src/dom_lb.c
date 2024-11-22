#include "dom_lb.h"

#include <stdlib.h>
#include <math.h>

static inline int compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int dom_sum_degree_bound(graph *g)
{
    int *D = malloc(sizeof(int) * g->n);
    for (int i = 0; i < g->n; i++)
        D[i] = (g->V[i + 1] - g->V[i]) + 1;

    qsort(D, g->n, sizeof(int), compare);

    int lb = 0, c = 0, j = g->n - 1;
    while (c < g->n)
    {
        lb++;
        c += D[j--];
    }

    free(D);

    return lb;
}

int dom_efficiency_bound(graph *g)
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