#include "hypergraph.h"
#include "graph_csr.h"
#include "reductions.h"
#include "chils.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

double get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

int name_offset(char *name)
{
    int offset = 0, i = 0;
    while (name[i] != '\0')
    {
        if (name[i] == '/')
            offset = i + 1;
        i++;
    }
    return offset;
}

int main(int argc, char **argv)
{
    double t0 = get_wtime();

    FILE *f = fopen(argv[1], "r");
    hypergraph *hg = hypergraph_parse(f);
    hypergraph *hgc = hypergraph_copy(hg);
    fclose(f);

    hypergraph_sort(hg);

    if (!hypergraph_validate(hg))
        printf("Error in graph\n");

    int nr = 1, lc = 0;
    while (nr > 0)
    {
        nr = 0;
        nr += reduction_degree_one_rule(hg);
        nr += reduction_vertex_domination(hg);
        nr += reduction_edge_domination(hg);
        lc++;
    }

    double t1 = get_wtime();

    int in = 0, out = 0, remaining = 0;
    for (int u = 0; u < hg->n; u++)
    {
        if (hg->Vd[u] == 0)
            out++;
        else if (hg->Vd[u] == 1)
            in++;
        else
            remaining++;
    }

    // printf("After reductions, %d %d %d\n", in, out, remaining);

    if (!hypergraph_validate(hg))
        printf("Error in graph after reductions\n");

    int *FM = malloc(sizeof(int) * hg->n);
    long long offset;
    graph_csr *g = graph_csr_construct(hg, FM, (1 << 10), &offset);
    chils *c = chils_init(g, 8, time(NULL));

    // printf("%10d %10d\n", g->n, g->V[g->n]);

    c->step_time = 1.0;
    chils_run(g, c, 20, 9999999, offset, 1);

    int *IS = chils_get_best_independent_set(c);

    int *HS = malloc(sizeof(int) * hg->n);
    for (int i = 0; i < hg->n; i++)
    {
        if (FM[i] >= 0)
            HS[i] = !IS[FM[i]];
        else
        {
            assert(hg->Vd[i] == 0 || hg->Vd[i] == 1);
            HS[i] = hg->Vd[i] == 1;
        }
    }

    int hs = 0;
    for (int i = 0; i < hg->n; i++)
    {
        if (HS[i])
            hs++;
    }

    int uc = 0;
    for (int e = 0; e < hgc->m; e++)
    {
        int any = 0;
        for (int i = 0; i < hgc->Ed[e]; i++)
        {
            int u = hgc->E[e][i];
            if (HS[u])
                any = 1;
        }
        if (!any)
        {
            uc++;
        }
    }

    printf("%25s,%10d,%10d\n", argv[1] + name_offset(argv[1]), hs, hs + uc);

    if (!graph_csr_validate(g))
        printf("Error in graph\n");

    free(HS);
    chils_free(c);
    free(FM);
    graph_csr_free(g);
    hypergraph_free(hgc);
    hypergraph_free(hg);

    return 0;
}