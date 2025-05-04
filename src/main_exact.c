#include "hypergraph.h"
#include "graph.h"
#include "reductions.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

double get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

int main(int argc, char **argv)
{
    double t0 = get_wtime();

    FILE *f = fopen(argv[1], "r");
    hypergraph *hg = hypergraph_parse(f);
    fclose(f);

    hypergraph_sort(hg);

    int offset = 0, i = 0;
    while (argv[1][i] != '\0')
    {
        if (argv[1][i] == '/')
            offset = i + 1;
        i++;
    }

    if (!hypergraph_validate(hg))
        printf("Error in graph\n");

    int nr = 1, lc = 0;
    while (nr > 0)
    {
        nr = 0;
        nr += reduction_vertex_domination(hg);
        nr += reduction_degree_one_rule(hg);
        nr += reduction_edge_domination(hg);
        // nr += reduction_counting_rule(hg);
        // nr += reduction_large_edge_rule(hg);
        lc++;
    }

    double t1 = get_wtime();

    int mde = 0;
    long long total_de = 0;
    for (int i = 0; i < hg->m; i++)
    {
        total_de += hg->Ed[i] > 1 ? hg->Ed[i] : 0;
        if (hg->Ed[i] > mde)
            mde = hg->Ed[i];
    }

    int mdv = 0;
    long long total_dv = 0;
    for (int i = 0; i < hg->n; i++)
    {
        total_dv += hg->Vd[i] > 1 ? hg->Vd[i] : 0;
        if (hg->Vd[i] > mdv)
            mdv = hg->Vd[i];
    }

    int rv = 0, re = 0;
    for (int i = 0; i < hg->n; i++)
    {
        if (hg->Vd[i] > 1)
            rv++;
    }
    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] > 1 && hg->Ed[i] < 16)
            re++;
    }

    if (!hypergraph_validate(hg))
        printf("Error\n");

    printf("%10s %9d (%9d) %5d (%8.2lf) %9d (%9d) %5d (%8.2lf) %8.4lf %3d\n",
           argv[1] + offset,
           hg->n, rv, mdv, (double)total_dv / (double)rv,
           hg->m, re, mde, (double)total_de / (double)re,
           t1 - t0, lc);

    // long long ds_offset;
    // graph *g = reduction_hitting_set_to_mwis(hg, &ds_offset);

    // reducer *r = reducer_init(g, 3,
    //                           degree_zero_reduction,
    //                           degree_one_reduction,
    //                           domination_reduction);

    // reduction_log *l = reducer_reduce(r, g);

    // int m = 0;
    // for (int u = 0; u < g->n; u++)
    // {
    //     if (!g->A[u])
    //         continue;
    //     m += g->D[u];
    // }

    // printf("%20s %10d %10d\n", argv[1] + offset, g->r_n, m / 2);

    // reducer_free_reduction_log(l);
    // reducer_free(r);

    // printf("%10s %9d %9d\n", argv[1] + offset, g->n, g->m / 2);

    // printf("%lld\n", ds_offset);

    // f = fopen("test.gr", "w");
    // fprintf(f, "%d %d %d\n", g->n, g->m / 2, 10);
    // for (int i = 0; i < g->n; i++)
    // {
    //     fprintf(f, "%lld", g->W[i]);
    //     for (int j = 0; j < g->D[i]; j++)
    //     {
    //         fprintf(f, " %d", g->V[i][j] + 1);
    //     }
    //     fprintf(f, "\n");
    // }
    // fclose(f);

    // graph_free(g);
    hypergraph_free(hg);

    return 0;
}