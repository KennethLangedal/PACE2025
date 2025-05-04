#include "reductions.h"

#include <assert.h>
#include <stdlib.h>

// Test if A is a subset of B
static inline int test_subset(const int *A, int a, const int *B, int b)
{
    if (b < a)
        return 0;

    int i = 0, j = 0;
    while (i < a && j < b)
    {
        if (A[i] == B[j])
        {
            i++;
            j++;
        }
        else if (A[i] > B[j])
        {
            j++;
        }
        else
        {
            return 0;
        }
    }

    return i == a;
}

int reduction_degree_one_rule(hypergraph *g)
{
    int r = 0;
    for (int u = 0; u < g->n; u++)
    {
        if (g->Vd[u] != 1)
            continue;

        int e = g->V[u][0];
        if (g->Ed[e] > 1)
        {
            r++;
            hypergraph_remove_vertex(g, u);
        }
    }
    for (int e = 0; e < g->m; e++)
    {
        if (g->Ed[e] != 1)
            continue;

        int u = g->E[e][0];
        if (g->Vd[u] > 1)
        {
            r++;
            hypergraph_include_vertex(g, u);
        }
    }
    return r;
}

int reduction_vertex_domination(hypergraph *g)
{
    int r = 0;
    for (int i = 0; i < g->n; i++)
    {
        int md = -1;
        for (int j = 0; j < g->Vd[i]; j++)
        {
            int e = g->V[i][j];
            if (md < 0 || g->Ed[e] < g->Ed[md])
                md = e;
        }
        if (md < 0 || g->Ed[md] > (1 << 10))
            continue;

        for (int j = 0; j < g->Ed[md]; j++)
        {
            int v = g->E[md][j];
            if (v == i)
                continue;

            if (test_subset(g->V[i], g->Vd[i], g->V[v], g->Vd[v]))
            {
                r++;
                hypergraph_remove_vertex(g, i);
                break;
            }
        }
    }
    return r;
}

int reduction_edge_domination(hypergraph *g)
{
    int r = 0;
    for (int e = 0; e < g->m; e++)
    {
        int md = -1;
        for (int i = 0; i < g->Ed[e]; i++)
        {
            int v = g->E[e][i];
            if (md < 0 || g->Vd[v] < g->Vd[md])
                md = v;
        }
        if (md < 0 || g->Vd[md] > (1 << 10))
            continue;

        for (int i = 0; i < g->Vd[md]; i++)
        {
            int e2 = g->V[md][i];
            if (e2 == e)
                continue;

            if (test_subset(g->E[e], g->Ed[e], g->E[e2], g->Ed[e2]))
            {
                r++;
                hypergraph_remove_edge(g, e2);
            }
        }
    }
    return r;
}

int reduction_counting_rule(hypergraph *g)
{
    int r = 0;
    int *fast_set = malloc(sizeof(int) * g->m);
    for (int i = 0; i < g->m; i++)
        fast_set[i] = -1;

    for (int u = 0; u < g->n; u++)
    {
        int c = 0, k = 0;
        for (int i = 0; i < g->Vd[u]; i++)
        {
            int e = g->V[u][i];
            if (g->Ed[e] != 2)
                continue;

            c++;

            int v = g->E[e][0] == u ? g->E[e][1] : g->E[e][0];

            for (int j = 0; j < g->Vd[v]; j++)
            {
                int e2 = g->V[v][j];
                if (e2 == e)
                    continue;

                if (fast_set[e2] != u)
                    k++;
                fast_set[e2] = u;
            }
        }

        if (k < c)
        {
            r++;
            hypergraph_include_vertex(g, u);
        }
    }
    free(fast_set);
    return r;
}

graph *reduction_hitting_set_to_mwis(hypergraph *hg, long long *offset)
{
    graph *g = graph_init();
    *offset = hg->n;

    for (int i = 0; i < hg->n; i++)
    {
        graph_add_vertex(g, 1);
    }

    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] == 0)
            continue;
        else if (hg->Ed[i] == 2)
        {
            graph_add_edge(g, hg->E[i][0], hg->E[i][1]);
            continue;
        }
        *offset += 1000;
        int s = g->n;
        for (int j = 0; j < hg->Ed[i]; j++)
        {
            graph_add_vertex(g, 1000);
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

    graph_sort_edges(g);

    return g;
}
