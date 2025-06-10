#include "hypergraph.h"
#include "graph_csr.h"
// #include "hs_reduction_to_mwis.h"
#include "chils.h"
#include "local_search.h"
#include "local_search_hs.h"
#include "hs_reductions/degree_one.h"
#include "hs_reductions/domination.h"
#include "hs_reductions/extended_domination.h"
#include "hs_reductions/counting_rule.h"
#include "hs_reducer.h"
#include "hs_reductions.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#define VERBOSE 0

volatile sig_atomic_t tle = 0;

void term(int signum)
{
    tle = 1;
}

static inline double get_wtime()
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

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    FILE *f = fopen(argv[1], "r");
    hypergraph *hg = hypergraph_parse(f);
    fclose(f);

    hypergraph_sort(hg);

    int rc = 1;
    double tl = 80.0;
    while (rc > 0 && tl > (get_wtime() - t0))
    {
        rc = 0;
        rc += hs_reductions_degree_one_rule(hg, tl - (get_wtime() - t0));
        rc += hs_reductions_edge_domination(hg, tl - (get_wtime() - t0));
        rc += hs_reductions_vertex_domination(hg, tl - (get_wtime() - t0));
    }
    rc = 1;
    while (rc > 0)
    {
        rc = 0;
        rc += hs_reductions_degree_one_rule(hg, 10.0);
    }

    int nr = 0, mr = 0, md = 0;
    for (int i = 0; i < hg->n; i++)
    {
        if (hg->Vd[i] >= 2)
            nr++;
    }
    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] >= 2)
            mr++;
        if (hg->Ed[i] > md)
            md = hg->Ed[i];
    }

    if (VERBOSE)
        printf("HS |V|=%10d (%10d) |E|=%10d (%10d) in %10.3lf (%10d)\n", nr, hg->n, mr, hg->m, get_wtime() - t0, md);

    long long offset = 0;
    int *FM_HS = malloc(sizeof(int) * hg->n);
    graph_csr *gh = graph_csr_construct_hypergraph(hg, FM_HS);

    local_search_hs *ls_hs = local_search_hs_init(gh, 0);

    long long m = 0;
    for (int i = 0; i < hg->n; i++)
        m += hg->Vd[i];

    for (int i = 0; i < hg->m; i++)
        m += (hg->Ed[i] * hg->Ed[i]) / 2;

    int stored_mwis = 0;
    if ((nr > 5000 && md < 32 && m < 5000000))
    {
        stored_mwis = 1;

        int *FM_MWIS = malloc(sizeof(int) * hg->n);
        graph *gr = hs_reductions_to_mwis(hg, FM_MWIS, 32, &offset); // (1 << 2)

        if (VERBOSE)
            printf("IS |V|=%lld |E|=%lld offset=%lld\n", gr->nr, gr->m, offset);

        void *rd = mwis_reduction_run_struction(gr, 150.0 - (get_wtime() - t0));

        if (VERBOSE)
            printf("%lld %lld\n", gr->nr, gr->m);

        offset -= mwis_reduction_get_offset(rd);

        int *I = NULL;
        if (gr->nr == 0)
        {
            I = mwis_reduction_lift_solution(NULL, rd);

            if (VERBOSE)
                printf("%lld\n", offset);
        }
        else
        {
            double tr = 10.0;
            mwis_reduction_dinsify(gr, rd, tr);

            if (VERBOSE)
                printf("After densify %lld %lld\n", gr->nr, gr->m);

            int *FM = malloc(sizeof(int) * gr->n);
            graph_csr *g = graph_csr_construct(gr, FM);

            // Store Graph
            char graph_name[256];
            int p = name_offset(argv[1]), j = 0;
            while (argv[1][p] != '.')
                graph_name[j++] = argv[1][p++];
            graph_name[j++] = '.';
            graph_name[j++] = 'g';
            graph_name[j++] = 'r';
            graph_name[j++] = '\0';
            f = fopen(graph_name, "w");
            fprintf(f, "%d %d %d\n", g->n, g->V[g->n], 10);
            for (int u = 0; u < g->n; u++)
            {
                fprintf(f, "%lld", g->W[u]);
                for (int i = g->V[u]; i < g->V[u + 1]; i++)
                {
                    int v = g->E[i];
                    fprintf(f, " %d", v + 1);
                }
                fprintf(f, "\n");
            }
            fclose(f);

            if (gr->nr > 100000)
            {
                tr = 3600.0 - (get_wtime() - t0);
                local_search *ls = local_search_init(g, 0);
                local_search_explore(g, ls, tr, &tle, LLONG_MAX, offset, VERBOSE);

                // Store Solution
                char sol_name[256];
                int p = name_offset(argv[1]);
                j = 0;
                while (argv[1][p] != '.')
                    sol_name[j++] = argv[1][p++];
                sol_name[j++] = '.';
                sol_name[j++] = 'i';
                sol_name[j++] = 's';
                sol_name[j++] = '\0';
                f = fopen(sol_name, "w");
                for (int u = 0; u < g->n; u++)
                {
                    fprintf(f, "%d\n", ls->independent_set[u]);
                }
                fclose(f);

                I = mwis_reduction_lift_solution(ls->independent_set, rd);
                local_search_free(ls);
            }
            else
            {
                tr = 3600.0 - (get_wtime() - t0);
                chils *c = chils_init(g, 8, 0);
                c->step_time = 2.5;
                chils_run(g, c, tr, &tle, LLONG_MAX, offset, VERBOSE);

                // Store Solution
                char sol_name[256];
                int p = name_offset(argv[1]);
                j = 0;
                while (argv[1][p] != '.')
                    sol_name[j++] = argv[1][p++];
                sol_name[j++] = '.';
                sol_name[j++] = 'i';
                sol_name[j++] = 's';
                sol_name[j++] = '\0';
                f = fopen(sol_name, "w");
                int *rI = chils_get_best_independent_set(c);
                for (int u = 0; u < g->n; u++)
                {
                    fprintf(f, "%d\n", rI[u]);
                }
                fclose(f);

                I = mwis_reduction_lift_solution(chils_get_best_independent_set(c), rd);
                chils_free(c);
            }

            free(FM);
            graph_csr_free(g);

            graph_free(gr);
            mwis_reduction_free(rd);
        }

        for (int e = 0; e < hg->m; e++)
        {
            if (hg->Ed[e] == 0)
                continue;
            int hit = 0;
            for (int i = 0; i < hg->Ed[e]; i++)
            {
                int u = hg->E[e][i];
                if (!I[u])
                    hit = 1;
            }
            if (!hit)
            {
                I[FM_MWIS[hg->E[e][0]]] = 0;
            }
        }

        for (int u = 0; u < hg->n; u++)
        {
            if (FM_HS[u] >= 0 && I[FM_MWIS[u]])
            {
                local_search_hs_remove_vertex(gh, ls_hs, FM_HS[u]);
            }
        }

        free(FM_MWIS);
        free(I);
    }

    offset = 0;
    for (int u = 0; u < hg->n; u++)
    {
        if (hg->Vd[u] == 1)
            offset++;
    }

    local_search_hs_explore(gh, ls_hs, 3700.0 - (get_wtime() - t0), &tle, offset, VERBOSE);

    if (!stored_mwis)
    {
        // Store Graph
        char graph_name[256];
        int p = name_offset(argv[1]), j = 0;
        while (argv[1][p] != '.')
            graph_name[j++] = argv[1][p++];
        graph_name[j++] = '.';
        graph_name[j++] = 'g';
        graph_name[j++] = 'r';
        graph_name[j++] = '\0';
        f = fopen(graph_name, "w");
        fprintf(f, "%d %d %d\n", gh->n + gh->m, gh->V[gh->n + gh->m], 10);
        for (int u = 0; u < gh->n; u++)
        {
            fprintf(f, "%d", 1);
            for (int i = gh->V[u]; i < gh->V[u + 1]; i++)
            {
                int v = gh->E[i];
                fprintf(f, " %d", gh->n + v + 1);
            }
            fprintf(f, "\n");
        }
        for (int e = 0; e < gh->m; e++)
        {
            fprintf(f, "%d", 0);
            for (int i = gh->V[gh->n + e]; i < gh->V[gh->n + e + 1]; i++)
            {
                int v = gh->E[i];
                fprintf(f, " %d", v + 1);
            }
            fprintf(f, "\n");
        }
        fclose(f);

        // Store Solution
        char sol_name[256];
        p = name_offset(argv[1]);
        j = 0;
        while (argv[1][p] != '.')
            sol_name[j++] = argv[1][p++];
        sol_name[j++] = '.';
        sol_name[j++] = 'h';
        sol_name[j++] = 's';
        sol_name[j++] = '\0';
        f = fopen(sol_name, "w");
        for (int u = 0; u < gh->n; u++)
        {
            fprintf(f, "%d\n", ls_hs->hitting_set[u]);
        }
        fclose(f);
    }

    if (!VERBOSE)
    {
        printf("%20s:%12lld\n", argv[1] + name_offset(argv[1]), ls_hs->cost + offset);
        // for (int u = 0; u < hg->n; u++)
        // {
        //     if (hg->Vd[u] == 1 || ls_hs->hitting_set[FM_HS[u]])
        //         printf("%d\n", u + 1);
        // }
    }

    free(FM_HS);
    graph_csr_free(gh);
    local_search_hs_free(ls_hs);

    hypergraph_free(hg);

    return 0;
}

// FILE *f = fopen("test.gr", "w");
// fprintf(f, "%lld %lld %d\n", gr->n, gr->m, 10);
// for (int i = 0; i < gr->n; i++)
// {
//     fprintf(f, "%lld", gr->W[i]);
//     for (int j = 0; j < gr->D[i]; j++)
//         fprintf(f, " %d", gr->V[i][j] + 1);
//     fprintf(f, "\n");
// }
// fclose(f);

// return 0;

// for (int i = 0; i < hg->m; i++)
// {
//     if (hg->Ed[i] < 2)
//         continue;
//     int any = 0;
//     for (int j = 0; j < hg->Ed[i]; j++)
//     {
//         int v = FM_HS[hg->E[i][j]];
//         if (ls_hs->hitting_set[v])
//             any = 1;
//     }
//     if (!any)
//     {
//         printf("Error in solution\n");
//     }
// }
// int hs = 0;
// for (int i = 0; i < hg->n; i++)
// {
//     if (hg->Vd[i] < 2)
//         continue;
//     int v = FM_HS[i];
//     hs += ls_hs->hitting_set[v];
// }
// printf("%lld\n", hs + offset);