#include "hs_reductions/counting_rule.h"

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

int hs_reductions_counting_rule_reduce_graph(hypergraph *g, int ue, int apply_on_edges, hs_change_list *c, int *fast_set, int fs_count)
{
    if (apply_on_edges)
    {
        int e = ue;
        int md = -1;
        for (int i = 0; i < g->Ed[e]; i++)
        {
            int v = g->E[e][i];
            if (md < 0 || g->Vd[v] < g->Vd[md])
                md = v;
        }
        if (md < 0 || g->Vd[md] > (1 << 10))
            return 0;

        for (int i = 0; i < g->Vd[md]; i++)
        {
            int e2 = g->V[md][i];
            if (e2 == e)
                continue;

            if (test_subset(g->E[e], g->Ed[e], g->E[e2], g->Ed[e2]))
            {
                hs_reducer_queue_up_neighbors_e(g, e2, c);
                hypergraph_remove_edge(g, e2);
                return 1;
            }
        }
    }
    return 0;
}