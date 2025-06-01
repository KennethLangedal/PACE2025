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

    hypergraph *hg = hypergraph_parse(stdin);

    hypergraph_sort(hg);

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

    long long offset;
    graph *gr = hs_reductions_to_mwis(hg, (1 << 7), &offset);

    double t1 = get_wtime();

    void *rd = mwis_reduction_run_struction(gr, 150.0 - (t1 - t0));

    offset -= mwis_reduction_get_offset(rd);

    int *I = NULL;
    if (gr->nr == 0)
    {
        I = mwis_reduction_lift_solution(NULL, rd);
    }
    else
    {
        int *FM = malloc(sizeof(int) * gr->n);
        graph_csr *g = graph_csr_construct(gr, FM);

        if (gr->nr > 200000 && gr->m > 1000000)
        {
            double tr = 60.0;
            local_search *ls = local_search_init(g, 0);
            local_search_explore(g, ls, tr, &tle, LLONG_MAX, offset, 0);

            I = mwis_reduction_lift_solution(ls->independent_set, rd);
            local_search_free(ls);
        }
        else
        {
            double tr = 240.0 - (get_wtime() - t0);
            chils *c = chils_init(g, 8, 0);
            c->step_time = 2.0;
            chils_run(g, c, tr, &tle, LLONG_MAX, offset, 0);

            I = mwis_reduction_lift_solution(chils_get_best_independent_set(c), rd);
            chils_free(c);
        }

        free(FM);
        graph_csr_free(g);
    }

    offset = 0;
    int *FM_HS = malloc(sizeof(int) * hg->n);
    graph_csr *gh = graph_csr_construct_hypergraph(hg, FM_HS);

    local_search_hs *ls_hs = local_search_hs_init(gh, 0);

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
            I[hg->E[e][0]] = 0;
        }
    }

    for (int u = 0; u < hg->n; u++)
    {
        if (hg->Vd[u] == 1)
            offset++;
        if (FM_HS[u] >= 0 && I[u])
        {
            local_search_hs_remove_vertex(gh, ls_hs, FM_HS[u]);
        }
    }

    local_search_hs_explore(gh, ls_hs, 600.0, &tle, LLONG_MAX, offset, 0);

    printf("%lld\n", ls_hs->cost + offset);

    for (int u = 0; u < hg->n; u++)
    {
        if (hg->Vd[u] == 1 || ls_hs->hitting_set[FM_HS[u]])
            printf("%d\n", u + 1);
    }

    free(FM_HS);
    graph_csr_free(gh);
    local_search_hs_free(ls_hs);

    hypergraph_free(hg);

    return 0;
}