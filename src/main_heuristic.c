#include "hypergraph.h"
#include "graph_csr.h"
#include "hs_reductions.h"
#include "chils.h"
#include "local_search.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>

volatile sig_atomic_t tle = 0;

void term(int signum)
{
    tle = 1;
}

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

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    // FILE *f = fopen(argv[1], "r");
    // hypergraph *hg = hypergraph_parse(f);
    // fclose(f);

    hypergraph *hg = hypergraph_parse(stdin);

    hypergraph_sort(hg);

    int nr = 1, lc = 0;
    while (nr > 0 && lc < 10)
    {
        nr = 0;
        nr += hs_reductions_degree_one_rule(hg);
        nr += hs_reductions_vertex_domination(hg);
        nr += hs_reductions_edge_domination(hg);
        // nr += hs_reductions_counting_rule(hg);
        lc++;
    }

    long long offset;
    graph *gr = hs_reductions_to_mwis(hg, (1 << 7), &offset);

    double t1 = get_wtime();

    // printf("%lld\n", gr->nr);
    void *rd = mwis_reduction_run_struction(gr, 100.0 - (t1 - t0));
    // printf("%lld\n", gr->nr);

    offset -= mwis_reduction_get_offset(rd);

    int *I = NULL;
    if (gr->nr == 0)
    {
        for (node_id i = 0; i < gr->n; i++)
            I[i] = 0;
        I = mwis_reduction_lift_solution(NULL, rd);
    }
    else
    {
        int *FM = malloc(sizeof(int) * gr->n);
        graph_csr *g = graph_csr_construct(gr, FM);
        double tr = 600.0 - (get_wtime() - t0);

        if (1 || gr->nr > 100000 && gr->m > 1000000)
        {
            // printf("%d\n", g->n);
            local_search *ls = local_search_init(g, 0);
            local_search_explore(g, ls, tr, &tle, 999999999ll, offset, 0);

            I = mwis_reduction_lift_solution(ls->independent_set, rd);
            local_search_free(ls);
        }
        else
        {
            printf("%d\n", g->n);
            chils *c = chils_init(g, 4, 0);
            c->step_time = 1.0;
            chils_run(g, c, tr, &tle, 999999999, offset, 0);

            I = mwis_reduction_lift_solution(chils_get_best_independent_set(c), rd);
            chils_free(c);
        }

        free(FM);
        graph_csr_free(g);
    }

    long long HS = 0;
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
        if (!I[u])
            HS++;
    }
    printf("%lld\n", HS);
    for (int u = 0; u < hg->n; u++)
    {
        if (!I[u])
            printf("%d\n", u + 1);
    }

    double t2 = get_wtime();

    // printf("%25s,%10lld,%10.2lf\n", argv[1] + name_offset(argv[1]), HS, t2 - t0);

    mwis_reduction_free(rd);
    graph_free(gr);
    free(I);
    hypergraph_free(hg);

    return 0;
}