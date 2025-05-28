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
    graph *g = hs_reductions_to_mwis(hg, (1 << 10), &offset);

    // int* MWIS_sol = maxsat_solve_MWIS(g);

    // printf("%lld %lld\n", g->n, g->m);
    void *rd = mwis_reduction_run_struction(g, 300);
    // void *rd = mwis_reduction_reduce_graph(g);
    // printf("%lld %lld\n", g->n, g->m);

    offset -= mwis_reduction_get_offset(rd);

    double t2 = get_wtime();

    clique_set *cs = find_cliques(g, 500, g->n);
    /* for (int i = 0; i < cs->num_cliques; i++) {
        printf("Clique %d (size %d): ", i + 1, cs->sizes[i]);
        for (int j = 0; j < cs->sizes[i]; j++) {
            printf("%lld ", (long long)cs->cliques[i][j]);
        }
        printf("\n");
    } */

    // int *MWIS_sol = maxsat_solve_MWIS_with_cliques(g, cs);
    int *MWIS_sol = maxsat_solve_MWIS(g);

    MWIS_sol = mwis_reduction_lift_solution(MWIS_sol, rd);

    long long HS = 0;
    for (int u = 0; u < hg->n; u++)
    {
        if (!MWIS_sol[u])
            HS++;
    }
    /* printf("%lld\n", HS);
    for (int u = 0; u < hg->n; u++)
    {
        if (!MWIS_sol[u])
            printf("%d\n", u + 1);
    } */

    printf("%lld,%.2f\n", HS, t2-t0);

    free(MWIS_sol);
    free_clique_set(cs);
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

    hypergraph_free(hg);

    return 0;
}