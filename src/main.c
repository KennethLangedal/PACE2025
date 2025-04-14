#include "hypergraph.h"
#include "graph.h"
#include "reductions.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "r");
    hypergraph *hg = hypergraph_parse(f);
    fclose(f);

    hypergraph_sort(hg);

    int offset = 0, i = 0;
    while (argv[1][i] != '\0')
    {
        if (argv[1][i] == '/')
            offset = i + 1;
        i++;
    }

    if (!hypergraph_validate(hg))
        printf("Error in graph\n");

    int r = 1;
    while (r > 0)
    {
        r = 0;
        r += reduction_vertex_domination(hg);
        r += reduction_edge_domination(hg);
    }

    int md = 0;
    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] > md)
            md = hg->Ed[i];
    }

    int rv = 0, re = 0;
    for (int i = 0; i < hg->n; i++)
    {
        if (hg->Vd[i] > 0)
            rv++;
    }
    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] > 0)
            re++;
    }

    if (!hypergraph_validate(hg))
        printf("Error\n");

    // printf("%10s %9d (%9d) %9d (%9d) %d\n", argv[1] + offset, hg->n, rv, hg->m, re, md);

    graph *g = reduction_hitting_set_to_mwis(hg);

    printf("%10s %9d %9d\n", argv[1] + offset, g->n, g->m / 2);

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

    graph_free(g);
    hypergraph_free(hg);

    return 0;
}