#include "hypergraph.h"
#include "hs_reductions.h"
#include "mwis_reductions.h"
#include "maxsat.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

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

    // FILE *f = fopen(argv[1], "r");
    // hypergraph *hg = hypergraph_parse(f);
    // close(f);

    hypergraph *hg = hypergraph_parse(stdin);
    
    hypergraph_sort(hg);

    int nr = 1, lc = 0;
    while (nr > 0)
    {
        nr = 0;
        nr += hs_reductions_vertex_domination(hg);
        nr += hs_reductions_degree_one_rule(hg);
        nr += hs_reductions_edge_domination(hg);
        // nr += hs_reductions_counting_rule(hg);
        lc++;
    }

    double t1 = get_wtime();

    /* long long hs_sol = maxsat_solve_hitting_set(hg);
    printf("%lld,%.2f\n", hs_sol, t1-t0); */

    long long offset;
    graph *g = hs_reductions_to_mwis(hg, (1 << 7), &offset);

    /* f = fopen("test.gr", "w");
    fprintf(f, "%lld %lld %d\n", g->n, g->m, 10);
    for (int i = 0; i < g->n; i++)
    {
        fprintf(f, "%lld", g->W[i]);
        for (int j = 0; j < g->D[i]; j++)
        {
            fprintf(f, " %d", g->V[i][j] + 1);
        }
        fprintf(f, "\n");
    }
    fclose(f);

    printf("%lld\n", offset);

    return 0; */

    // printf("%lld %lld\n", g->n, g->m);
    void *rd = mwis_reduction_run_struction(g, 300);
    // void *rd = mwis_reduction_reduce_graph(g);
    // printf("%lld %lld\n", g->n, g->m);

    offset -= mwis_reduction_get_offset(rd);

    double t2 = get_wtime();

    long long hs_sol = maxsat_solve_MWIS(g, offset);
    printf("%lld,%.2f\n", hs_sol, t2-t0);

    mwis_reduction_free(rd);
    graph_free(g);

    /* double t3 = get_wtime();

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
           argv[1] + name_offset(argv[1]),
           hg->n, rv, mdv, (double)total_dv / (double)rv,
           hg->m, re, mde, (double)total_de / (double)re,
           t3 - t0, lc); */

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