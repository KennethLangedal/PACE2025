#include "hypergraph.h"
#include "reductions.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "r");
    hypergraph *g = hypergraph_parse(f);
    fclose(f);

    hypergraph_sort(g);

    int offset = 0, i = 0;
    while (argv[1][i] != '\0')
    {
        if (argv[1][i] == '/')
            offset = i + 1;
        i++;
    }

    if (!hypergraph_validate(g))
        printf("Error in graph\n");

    int r = 1;
    while (r > 0)
    {
        r = 0;
        r += reduction_vertex_domination(g);
        r += reduction_edge_domination(g);
    }

    int md = 0;
    for (int i = 0; i < g->m; i++)
    {
        if (g->Ed[i] > md)
            md = g->Ed[i];
    }

    int rv = 0, re = 0;
    for (int i = 0; i < g->n; i++)
    {
        if (g->Vd[i] > 0)
            rv++;
    }
    for (int i = 0; i < g->m; i++)
    {
        if (g->Ed[i] > 0)
            re++;
    }

    if (!hypergraph_validate(g))
        printf("Error\n");

    printf("%10s %9d (%9d) %9d (%9d) %d\n", argv[1] + offset, g->n, rv, g->m, re, md);

    hypergraph_free(g);

    return 0;
}