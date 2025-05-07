#include "hypergraph.h"
#include "graph_csr.h"
#include "hs_reductions.h"
#include "chils.h"

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
    hypergraph *hgc = hypergraph_copy(hg);
    fclose(f);

    hypergraph_sort(hg);

    if (!hypergraph_validate(hg))
        printf("Error in graph\n");

    int nr = 1, lc = 0;
    while (nr > 0)
    {
        nr = 0;
        nr += hs_reductions_degree_one_rule(hg);
        nr += hs_reductions_vertex_domination(hg);
        nr += hs_reductions_edge_domination(hg);
        nr += hs_reductions_counting_rule(hg);
        lc++;
    }

    if (!hypergraph_validate(hg))
        printf("Error in graph after reductions\n");

    long long offset;
    graph *gr = hs_reductions_to_mwis(hg, (1 << 8), &offset);

    printf("%lld %lld\n", gr->n, gr->m);
    void *rd = mwis_reduction_reduce_graph(gr);
    printf("%lld %lld\n", gr->n, gr->m);

    offset -= mwis_reduction_get_offset(rd);

    int *FM = malloc(sizeof(int) * gr->n);
    graph_csr *g = graph_csr_construct(gr, FM);

    if (!graph_csr_validate(g))
        printf("Error in csr graph\n");

    chils *c = chils_init(g, 8, time(NULL));
    c->step_time = 0.5;
    chils_run(g, c, 60, 9999999, offset, 1);

    double t1 = get_wtime();

    printf("%25s,%10lld\n", argv[1] + name_offset(argv[1]), offset - c->cost);

    if (!graph_csr_validate(g))
        printf("Error in graph\n");

    mwis_reduction_free(rd);
    chils_free(c);
    free(FM);
    graph_csr_free(g);
    graph_free(gr);
    hypergraph_free(hgc);
    hypergraph_free(hg);

    return 0;
}