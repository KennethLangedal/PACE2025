#include "graph_csr.h"

#include <stdlib.h>

static inline int graph_csr_compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

graph_csr *graph_csr_construct(graph *rg, int *FM)
{
    graph_csr *g = malloc(sizeof(graph_csr));

    int n = 0, m = 0;
    for (node_id u = 0; u < rg->n; u++)
    {
        if (!rg->A[u])
        {
            FM[u] = -1;
            continue;
        }
        FM[u] = n++;
        m += rg->D[u];
    }

    g->n = n;
    g->m = m;
    g->V = malloc(sizeof(int) * (n + 1));
    g->E = malloc(sizeof(int) * m);
    g->W = malloc(sizeof(long long) * n);

    m = 0;
    for (node_id u = 0; u < rg->n; u++)
    {
        if (!rg->A[u])
            continue;

        int _u = FM[u];
        g->V[_u] = m;
        g->W[_u] = rg->W[u];

        for (node_id i = 0; i < rg->D[u]; i++)
        {
            int v = rg->V[u][i];
            int _v = FM[v];

            g->E[m++] = _v;
        }
    }
    g->V[g->n] = m;

    return g;
}

graph_csr *graph_csr_construct_hypergraph(hypergraph *rg, int *FM)
{
    graph_csr *g = malloc(sizeof(graph_csr));

    int *FM_E = malloc(sizeof(int) * rg->m);

    int n = 0, m = 0, c = 0;
    for (int u = 0; u < rg->n; u++)
    {
        if (rg->Vd[u] <= 1)
        {
            FM[u] = -1;
            continue;
        }
        FM[u] = n++;
        c += rg->Vd[u];
    }
    for (int e = 0; e < rg->m; e++)
    {
        if (rg->Ed[e] <= 1)
            continue;

        FM_E[e] = m++;
        c += rg->Ed[e];
    }

    g->n = n;
    g->m = m;
    g->V = malloc(sizeof(int) * (g->n + g->m + 1));
    g->E = malloc(sizeof(int) * c);
    g->W = NULL;

    c = 0;
    for (int u = 0; u < rg->n; u++)
    {
        if (rg->Vd[u] <= 1)
            continue;

        int _u = FM[u];
        g->V[_u] = c;

        for (int i = 0; i < rg->Vd[u]; i++)
        {
            int e = rg->V[u][i];
            int _e = FM_E[e];

            g->E[c++] = _e;
        }
    }
    for (int e = 0; e < rg->m; e++)
    {
        if (rg->Ed[e] <= 1)
            continue;

        int _e = FM_E[e] + g->n;
        g->V[_e] = c;

        for (int i = 0; i < rg->Ed[e]; i++)
        {
            int u = rg->E[e][i];
            int _u = FM[u];

            g->E[c++] = _u;
        }
    }
    g->V[g->n + g->m] = c;

    free(FM_E);

    return g;
}

void graph_csr_free(graph_csr *g)
{
    free(g->V);
    free(g->E);
    free(g->W);

    free(g);
}

int graph_csr_validate(graph_csr *g)
{
    int m = 0;
    for (int u = 0; u < g->n; u++)
    {
        if (g->V[u + 1] - g->V[u] < 0)
            return 0;

        m += g->V[u + 1] - g->V[u];

        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            if (i < 0 || i >= g->V[g->n])
                return 0;

            int v = g->E[i];
            if (v < 0 || v >= g->n || v == u || (i > g->V[u] && v <= g->E[i - 1]))
                return 0;

            if (bsearch(&u, g->E + g->V[v], g->V[v + 1] - g->V[v], sizeof(int), graph_csr_compare) == NULL)
                return 0;
        }
    }

    if (m != g->m)
        return 0;

    return 1;
}

void graph_csr_subgraph(graph_csr *g, graph_csr *sg, int *Mask, int *RM, int *FM)
{
    int n = 0, m = 0;
    for (int u = 0; u < g->n; u++)
    {
        if (!Mask[u])
            continue;

        FM[u] = n;
        RM[n] = u;
        n++;

        for (int i = g->V[u]; i < g->V[u + 1]; i++)
            if (Mask[g->E[i]])
                m++;
    }

    sg->n = n;

    m = 0;
    for (int u = 0; u < g->n; u++)
    {
        if (!Mask[u])
            continue;

        sg->W[FM[u]] = g->W[u];
        sg->V[FM[u]] = m;

        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int v = g->E[i];
            if (!Mask[v])
                continue;

            sg->E[m] = FM[v];
            m++;
        }
    }
    sg->V[sg->n] = m;
}