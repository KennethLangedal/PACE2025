#include "hypergraph.h"
#include "hs_reductions.h"
#include "mwis_reductions.h"
#include "maxsat.h"
#include "connected_components.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

double get_wtime() {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double) tp.tv_sec + ((double) tp.tv_nsec / 1e9);
}

int name_offset(char *name) {
    int offset = 0, i = 0;
    while (name[i] != '\0') {
        if (name[i] == '/')
            offset = i + 1;
        i++;
    }
    return offset;
}

long long solve_hg(hypergraph *hg) {
    int nr = 1, lc = 0;
    while (nr > 0) {
        nr = 0;
        nr += hs_reductions_vertex_domination(hg);
        nr += hs_reductions_degree_one_rule(hg);
        nr += hs_reductions_edge_domination(hg);
        // nr += hs_reductions_counting_rule(hg);
        lc++;
    }

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

    /* clique_set *cs = find_cliques(g, 500, g->n);
    for (int i = 0; i < cs->num_cliques; i++) {
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
    for (int u = 0; u < hg->n; u++) {
        if (!MWIS_sol[u])
            HS++;
    }
    /* printf("%lld\n", HS);
    for (int u = 0; u < hg->n; u++)
    {
        if (!MWIS_sol[u])
            printf("%d\n", u + 1);
    } */

    free(MWIS_sol);
    // free_clique_set(cs);
    mwis_reduction_free(rd);
    graph_free(g);

    return HS;

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
}

int main(int argc, char **argv) {
    double t0 = get_wtime();

    // FILE *f = fopen(argv[1], "r");
    // hypergraph *hg = hypergraph_parse(f);
    // close(f);

    hypergraph *hg = hypergraph_parse(stdin);

    hypergraph_sort(hg);

    bool use_connected_comp = false;

    if (!use_connected_comp) {
        long long HS = 0;
        HS += solve_hg(hg);
        printf("%lld\n", HS);
        hypergraph_free(hg);
        return 0;
    }

    double t_components_start = get_wtime();

    int               n_components = 0;
    translation_table **vertex_tt  = NULL;
    translation_table **edge_tt    = NULL;
    hypergraph        **components = find_connected_components(hg, &n_components, &vertex_tt, &edge_tt);

    for (int i = 0; i < n_components; i++) {
        hypergraph_sort(components[i]);
    }

    double t_components_end = get_wtime();
    // printf("Found %d components in %.5f seconds\n", n_components, t_components_end - t_components_start);

    long long HS = 0;
    for (int  i  = 0; i < n_components; i++) {
        HS += solve_hg(components[i]);
    }
    printf("%lld\n", HS);

    for (int i = 0; i < n_components; i++) {
        hypergraph_free(components[i]);
        translation_table_free(vertex_tt[i]);
        translation_table_free(edge_tt[i]);
    }
    free(components);
    free(vertex_tt);
    free(edge_tt);

    hypergraph_free(hg);

    return 0;
}