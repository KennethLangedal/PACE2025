#include "reductions.h"

// Test if A is a subset of B
static inline int test_subset(const int *A, int a, const int *B, int b)
{
    if (b < a)
        return 0;

    int i = 0, j = 0;
    while (i < a && j < b)
    {
        if (A[i] > B[j])
        {
            j++;
        }
        else if (A[i] == B[j])
        {
            i++;
            j++;
        }
        else
        {
            return 0;
        }
    }

    return i == a;
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
        if (md < 0)
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
    for (int i = 0; i < g->m; i++)
    {
        int md = -1;
        for (int j = 0; j < g->Ed[i]; j++)
        {
            int v = g->E[i][j];
            if (md < 0 || g->Vd[v] < g->Vd[md])
                md = v;
        }
        if (md < 0)
            continue;

        for (int j = 0; j < g->Vd[md]; j++)
        {
            int e = g->V[md][j];
            if (e == i)
                continue;

            if (test_subset(g->E[i], g->Ed[i], g->E[e], g->Ed[e]))
            {
                r++;
                hypergraph_remove_edge(g, e);
            }
        }
    }
    return r;
}