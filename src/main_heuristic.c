#include "hypergraph.h"
#include "graph_csr.h"
#include "hs_reductions.h"
#include "chils.h"
#include "local_search.h"

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
    fclose(f);

    hypergraph_sort(hg);

    int nr = 1, lc = 0;
    while (nr > 0 && lc < 5)
    {
        nr = 0;
        nr += hs_reductions_degree_one_rule(hg);
        nr += hs_reductions_vertex_domination(hg);
        nr += hs_reductions_edge_domination(hg);
        // nr += hs_reductions_counting_rule(hg);
        lc++;
    }

    long long offset;
    graph *gr = hs_reductions_to_mwis(hg, (1 << 9), &offset);

    double t1 = get_wtime();

    printf("%lld %lld\n", gr->n, gr->m);
    void *rd = mwis_reduction_run_struction(gr, 30.0 - (t1 - t0));
    // void *rd = mwis_reduction_reduce_graph(gr);
    printf("%lld %lld\n", gr->n, gr->m);

    return 0;

    offset -= mwis_reduction_get_offset(rd);

    int *FM = malloc(sizeof(int) * gr->n);
    graph_csr *g = graph_csr_construct(gr, FM);

    if (!graph_csr_validate(g))
        printf("Error in csr graph\n");

    local_search *ls = local_search_init(g, 0);
    double tr = 300.0 - (get_wtime() - t0);
    local_search_explore(g, ls, tr, 999999999ll, offset, 0);

    // chils *c = chils_init(g, 8, 0);
    // double tr = 300.0 - (get_wtime() - t0);
    // c->step_time = 1.0;
    // chils_run(g, c, tr, 9999999, offset, 1);

    // int *I = mwis_reduction_lift_solution(chils_get_best_independent_set(c), rd);
    int *I = mwis_reduction_lift_solution(ls->independent_set, rd);
    // mwis_reduction_restore_graph(gr, rd);

    long long HS = 0;
    for (node_id u = 0; u < hg->n; u++)
        if (!I[u])
            HS++;

    double t2 = get_wtime();

    printf("%25s,%10lld,%10.2lf\n", argv[1] + name_offset(argv[1]), HS, t2 - t0);

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
            printf("Error in solution\n");
        }
    }

    if (!graph_csr_validate(g))
        printf("Error in graph\n");

    mwis_reduction_free(rd);
    // chils_free(c);
    local_search_free(ls);
    free(FM);
    graph_csr_free(g);
    graph_free(gr);
    free(I);
    hypergraph_free(hg);

    return 0;
}