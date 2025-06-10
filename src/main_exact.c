#include "hypergraph.h"
#include "maxsat.h"
#include "connected_components.h"
#include "hs_reductions/degree_one.h"
#include "hs_reductions/domination.h"
#include "hs_reductions/extended_domination.h"
#include "hs_reductions/counting_rule.h"
#include "hs_reducer.h"
#include "hs_reductions.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

double t_total = 0.0;

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

#define max(a, b) ((a) > (b) ? (a) : (b))

long long mwis_solve_hg(hypergraph *hg, int **sol, int reduce_option)
{
    // reduce_option: 0 - None, 1 - Struction, 2 - Reduce Graph
    long long offset;
    graph *g = hs_reductions_to_mwis(hg, (1 << 30), &offset);

    void *rd = NULL;
    if (reduce_option == 1)
    {
        rd = mwis_reduction_run_struction(g, 300);
    }
    else if (reduce_option == 2)
    {
        rd = mwis_reduction_reduce_graph(g, 60);
    }

    int *MWIS_sol = maxsat_solve_MWIS(g);

    if (reduce_option != 0)
    {
        MWIS_sol = mwis_reduction_lift_solution(MWIS_sol, rd);
        mwis_reduction_free(rd);
    }

    long long HS = 0;
    for (int u = 0; u < hg->n; u++)
    {
        if (!MWIS_sol[u])
            HS++;
    }

    int res_size = 0;
    *sol = (int *)malloc(HS * sizeof(int));

    for (int u = 0; u < hg->n; u++)
    {
        if (!MWIS_sol[u])
            (*sol)[res_size++] = u;
    }

    free(MWIS_sol);
    graph_free(g);

    return HS;
}

int get_max_e_deg(hypergraph *hg)
{
    int max_e_deg = 0;
    for (int e = 0; e < hg->m; ++e)
    {
        max_e_deg = max(max_e_deg, hg->Ed[e]);
    }
    return max_e_deg;
}

int get_max_v_deg(hypergraph *hg)
{
    int max_v_deg = 0;
    for (int v = 0; v < hg->n; ++v)
    {
        max_v_deg = max(max_v_deg, hg->Vd[v]);
    }
    return max_v_deg;
}

long long solve_hg(hypergraph *hg, bool is_one_component, int **sol)
{
    if (is_one_component)
    {
        // The hypergraph is one component so solve it

        hs_reductions_degree_one_rule(hg);
        hs_reductions_vertex_domination(hg);
        hs_reductions_edge_domination(hg);
        hs_reductions_degree_one_rule(hg);
        hs_reductions_vertex_domination(hg);
        hs_reductions_edge_domination(hg);

        hs_reducer *r = hs_reducer_init(hg, 2,
                                        hs_degree_one,
                                        domination);
        hs_reducer_reduce(r, hg);
        hs_reducer_free(r);

        if (hg->n == 1 && hg->m == 1)
        {
            *sol = (int *)malloc(1 * sizeof(int));
            (*sol)[0] = 0;
            return 1;
        }
        if (hg->n == 1 && hg->m == 0)
        {
            *sol = NULL;
            return 0;
        }

        long long HS = 0;
        if (are_multiple_components(hg))
        {
            HS = solve_hg(hg, false, sol);
        }
        else
        {
            if (get_max_e_deg(hg) <= 2 && false)
            {
                double t0 = get_wtime();
                HS = mwis_solve_hg(hg, sol, 0);
                double t1 = get_wtime();
                if (hg->n > 100)
                {
                    int max_v_deg = get_max_v_deg(hg);
                    int max_e_deg = get_max_e_deg(hg);
                    // printf("mwis solving a hg with %d vertices (max deg=%d) and %d edges (max deg=%d) in %f sec\n", hg->n, max_v_deg, hg->m, max_e_deg, t1 - t0);
                }
            }
            else
            {

                double t0 = get_wtime();
                // HS = maxsat_solve_hitting_set(hg, sol);
                HS = maxsat_solve_hitting_set_inmplicit(hg, sol);
                double t1 = get_wtime();
                t_total += t1 - t0;
                if (hg->n > 100)
                {
                    int max_v_deg = get_max_v_deg(hg);
                    int max_e_deg = get_max_e_deg(hg);
                    // printf("maxsat solving a hg with %d vertices (max deg=%d) and %d edges (max deg=%d) in %f sec (total: %f)\n", hg->n, max_v_deg, hg->m, max_e_deg, t1 - t0, t_total);
                }
            }
        }
        return HS;
    }
    else
    {
        // The hypergraph could be made out of multiple components, separate
        // them and solve each one separately

        double t0 = get_wtime();

        int n_components = 0;
        translation_table **vertex_tt = NULL;
        translation_table **edge_tt = NULL;
        int **comp_sol = NULL;
        int *comp_sol_size = NULL;
        hypergraph **components = find_connected_components(hg, &n_components, &vertex_tt, &edge_tt);

        double t1 = get_wtime();

        // printf("splitting hg with %d vertices into %d components in %f sec\n", hg->n, n_components, t1 - t0);

        for (int i = 0; i < n_components; i++)
        {
            hypergraph_sort(components[i]);
        }

        comp_sol      = (int **) malloc(n_components * sizeof(int *));
        comp_sol_size = (int *) malloc(n_components * sizeof(int));

        long long HS = 0;
        for (int  i  = 0; i < n_components; i++) {
            comp_sol_size[i] = solve_hg(components[i], true, &comp_sol[i]);
            HS += comp_sol_size[i];
        }

        int sol_size = 0;
        *sol = (int *) malloc(HS * sizeof(int));

        for (int i = 0; i < n_components; i++) {
            for (int j = 0; j < comp_sol_size[i]; ++j) {
                (*sol)[sol_size++] = get_old(vertex_tt[i], comp_sol[i][j]);
            }
        }

        for (int i = 0; i < n_components; i++) {
            hypergraph_free(components[i]);
            translation_table_free(vertex_tt[i]);
            translation_table_free(edge_tt[i]);
            free(comp_sol[i]);
        }
        free(components);
        free(vertex_tt);
        free(edge_tt);
        free(comp_sol);
        free(comp_sol_size);

        return HS;
    }

    // printf("%lld,%.2f\n", HS, t1-t0);

    /* long long offset;
    graph *g = hs_reductions_to_mwis(hg, (1 << 10), &offset); */

    // int* MWIS_sol = maxsat_solve_MWIS(g);

    // printf("%lld %lld\n", g->n, g->m);
    /* void *rd = mwis_reduction_run_struction(g, 300); */
    // void *rd = mwis_reduction_reduce_graph(g);
    // printf("%lld %lld\n", g->n, g->m);

    /* offset -= mwis_reduction_get_offset(rd); */

    /* clique_set *cs = find_cliques(g, 500, g->n);
    for (int i = 0; i < cs->num_cliques; i++) {
        printf("Clique %d (size %d): ", i + 1, cs->sizes[i]);
        for (int j = 0; j < cs->sizes[i]; j++) {
            printf("%lld ", (long long)cs->cliques[i][j]);
        }
        printf("\n");
    } */

    // int *MWIS_sol = maxsat_solve_MWIS_with_cliques(g, cs);
    /* int *MWIS_sol = maxsat_solve_MWIS(g);

    MWIS_sol = mwis_reduction_lift_solution(MWIS_sol, rd);

    long long HS = 0;
    for (int u = 0; u < hg->n; u++) {
        if (!MWIS_sol[u])
            HS++;
    } */
    /* printf("%lld\n", HS);
    for (int u = 0; u < hg->n; u++)
    {
        if (!MWIS_sol[u])
            printf("%d\n", u + 1);
    } */

    /* free(MWIS_sol);
    // free_clique_set(cs);
    mwis_reduction_free(rd);
    graph_free(g); */

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
    // fclose(f);

    hypergraph *hg = hypergraph_parse(stdin);

    hypergraph_sort(hg);

    long long HS   = 0;
    int       *sol = NULL;

    HS = solve_hg(hg, false, &sol);

    printf("%lld\n", HS);
    for (int i = 0; i < HS; ++i) {
        printf("%d\n", 1 + sol[i]);
    }

    hypergraph_free(hg);
    free(sol);
    return 0;
}