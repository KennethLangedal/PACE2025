#define _GNU_SOURCE

#include "hypergraph.h"
#include "graph_csr.h"

#include "chils.h"
#include "local_search.h"
#include "local_search_hs.h"
#include "hs_reductions.h"
#include "simulated_annealing.h"

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

int cmp_degree(const void *a, const void *b, void *c)
{
    graph_csr *g = (graph_csr *)c;
    int u = *((const int *)a);
    int v = *((const int *)b);
    return (g->V[v + 1] - g->V[v]) - (g->V[u + 1] - g->V[u]);
}

int main(int argc, char **argv)
{
    double t0 = get_wtime();

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    hypergraph *hg = hypergraph_parse(stdin);

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
        if (hg->Ed[i] > 2)
            m += (hg->Ed[i] * hg->Ed[i]) / 2;

    if ((nr > 5000 || md == 2) && md < 32 && m < 5000000)
    {
        int *FM_MWIS = malloc(sizeof(int) * hg->n);
        graph *gr = hs_reductions_to_mwis(hg, FM_MWIS, 32, &offset);

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
            if (gr->nr < 10000)
                mwis_reduction_dinsify(gr, rd, tr);

            if (VERBOSE)
                printf("After densify %lld %lld\n", gr->nr, gr->m);

            int *FM = malloc(sizeof(int) * gr->n);
            graph_csr *g = graph_csr_construct(gr, FM);

            if (gr->nr > 100000)
            {
                tr = 270.0 - (get_wtime() - t0);
                local_search *ls = local_search_init(g, 0);
                local_search_explore(g, ls, tr, &tle, LLONG_MAX, offset, VERBOSE);

                I = mwis_reduction_lift_solution(ls->independent_set, rd);
                local_search_free(ls);
            }
            else
            {
                tr = 270.0 - (get_wtime() - t0);
                chils *c = chils_init(g, 8, 0);
                c->step_time = 2.5;
                chils_run(g, c, tr, &tle, LLONG_MAX, offset, VERBOSE);

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
                int u = FM_MWIS[hg->E[e][i]];
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
    else if (mr < 20000 && mr > 0)
    {
        offset = 0;
        for (int u = 0; u < hg->n; u++)
        {
            if (hg->Vd[u] == 1)
                offset++;
        }

        double tr;

        simulated_annealing *sa = simulated_annealing_init(gh, 0);
        sa->k = 200000000ll;

        while (1)
        {
            simulated_annealing_reset(gh, sa);
            for (int j = 0; j < gh->n; j++)
                if (!sa->best_hitting_set[j])
                    simulated_annealing_remove_vertex(gh, sa, j);

            tr = 270.0 - (get_wtime() - t0);
            if (tr < 0.0)
                break;
            simulated_annealing_start(gh, sa, tr, &tle, offset, VERBOSE);
        }

        // Validate
        for (int e = 0; e < gh->m; e++)
        {
            int hit = 0;
            for (int i = gh->V[gh->n + e]; i < gh->V[gh->n + e + 1]; i++)
            {
                int u = gh->E[i];
                if (sa->best_hitting_set[u])
                    hit = 1;
            }
            assert(hit);
        }

        for (int u = 0; u < gh->n; u++)
        {
            if (!sa->best_hitting_set[u])
                local_search_hs_remove_vertex(gh, ls_hs, u);
        }

        local_search_hs_reset(gh, ls_hs);

        simulated_annealing_free(sa);
    }

    offset = 0;
    for (int u = 0; u < hg->n; u++)
    {
        if (hg->Vd[u] == 1)
            offset++;
    }

    if (gh->n < 5000 && gh->n > 0)
    {
        int *order = malloc(sizeof(int) * gh->n);
        for (int i = 0; i < gh->n; i++)
            order[i] = i;

        qsort_r(order, gh->n, sizeof(int), cmp_degree, gh);

        for (int t = 0; t < 5; t++)
        {
            ls_hs->log_enabled = 0;

            int top = 64;
            local_search_hs_shuffle(order, top, &ls_hs->seed);

            local_search_hs_reset(gh, ls_hs);

            for (int i = 0; i < top; i++)
            {
                int u = order[i];
                if (ls_hs->score[u] == 0)
                {
                    local_search_hs_exclude_vertex(gh, ls_hs, u);
                }
            }

            local_search_hs_explore(gh, ls_hs, 1.0, &tle, offset, VERBOSE);
        }

        free(order);
    }

    local_search_hs_reset(gh, ls_hs);

    for (int i = 0; i < gh->n; i++)
        if (!ls_hs->best_hitting_set[i])
            local_search_hs_remove_vertex(gh, ls_hs, i);

    if (gh->n > 0)
        local_search_hs_explore(gh, ls_hs, 350.0 - (get_wtime() - t0), &tle, offset, VERBOSE);

    if (!VERBOSE)
    {
        printf("%12lld\n", ls_hs->best_cost + offset);
        for (int u = 0; u < hg->n; u++)
        {
            if (hg->Vd[u] == 1 || ls_hs->best_hitting_set[FM_HS[u]])
                printf("%d\n", u + 1);
        }
    }

    free(FM_HS);
    graph_csr_free(gh);
    local_search_hs_free(ls_hs);

    hypergraph_free(hg);

    return 0;
}
