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

// graph_csr *graph_csr_construct(hypergraph *hg, int *FM, int max_edge, long long *offset)
// {
//     graph_csr *g = malloc(sizeof(graph_csr));

//     *offset = 0;
//     int n = 0, m = 0;
//     for (int u = 0; u < hg->n; u++)
//     {
//         if (hg->Vd[u] <= 1)
//         {
//             if (hg->Vd[u] == 1 && hg->Ed[hg->V[u][0]] == 1)
//                 *offset += 1;
//             FM[u] = -1;
//             continue;
//         }
//         FM[u] = n++;
//     }
//     *offset += n;
//     for (int e = 0; e < hg->m; e++)
//     {
//         int d = hg->Ed[e];
//         if (d <= 1 || d > max_edge)
//             continue;

//         if (d == 2)
//             m += 1;
//         else
//         {
//             n += d;
//             m += ((d * (d - 1)) / 2) + d;

//             *offset += 10000;
//         }
//     }

//     int *V = malloc(sizeof(int) * (n + 1));
//     int *D = malloc(sizeof(int) * n);
//     int *E = malloc(sizeof(int) * m * 2);
//     long long *W = malloc(sizeof(long long) * n);

//     int c = 0;
//     for (int u = 0; u < hg->n; u++)
//     {
//         if (hg->Vd[u] <= 1)
//             continue;
//         D[FM[u]] = 0;
//         c++;
//     }
//     for (int e = 0; e < hg->m; e++)
//     {
//         int d = hg->Ed[e];
//         if (d <= 1 || d > max_edge)
//             continue;

//         if (d == 2)
//         {
//             int v1 = hg->E[e][0], v2 = hg->E[e][1];
//             D[FM[v1]]++;
//             D[FM[v2]]++;
//         }
//         else
//         {
//             for (int i = 0; i < d; i++)
//             {
//                 D[FM[hg->E[e][i]]]++;
//                 D[c + i] = d;
//             }
//             c += d;
//         }
//     }

//     V[0] = 0;
//     for (int i = 1; i <= n; i++)
//         V[i] = V[i - 1] + D[i - 1];
//     for (int i = 0; i < n; i++)
//         D[i] = 0;

//     c = 0;
//     for (int u = 0; u < hg->n; u++)
//     {
//         if (hg->Vd[u] <= 1)
//             continue;
//         W[FM[u]] = 1;
//         c++;
//     }
//     for (int e = 0; e < hg->m; e++)
//     {
//         int d = hg->Ed[e];
//         if (d <= 1 || d > max_edge)
//             continue;

//         if (d == 2)
//         {
//             int v1 = FM[hg->E[e][0]], v2 = FM[hg->E[e][1]];
//             E[V[v1] + D[v1]++] = v2;
//             E[V[v2] + D[v2]++] = v1;
//         }
//         else
//         {
//             for (int i = 0; i < d; i++)
//             {
//                 int u = FM[hg->E[e][i]];
//                 W[c + i] = 10000;
//                 E[V[u] + D[u]++] = c + i;
//                 E[V[c + i] + D[c + i]++] = u;

//                 for (int j = i + 1; j < d; j++)
//                 {
//                     E[V[c + i] + D[c + i]++] = c + j;
//                     E[V[c + j] + D[c + j]++] = c + i;
//                 }
//             }
//             c += d;
//         }
//     }

//     for (int u = 0; u < n; u++)
//         qsort(E + V[u], V[u + 1] - V[u], sizeof(int), graph_csr_compare);

//     m = 0;
//     for (int u = 0; u < n; u++)
//     {
//         int s = V[u];
//         V[u] = m;
//         for (int i = s; i < V[u + 1]; i++)
//         {
//             if (i == s || E[i] > E[i - 1])
//                 E[m++] = E[i];
//         }
//     }
//     V[n] = m;
//     E = realloc(E, sizeof(int) * m);

//     free(D);

//     *g = (graph_csr){.n = n, .V = V, .E = E, .W = W};
//     return g;
// }

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

    if (m != g->V[g->n])
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