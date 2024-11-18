#include "graph.h"
#include "dom_lb.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *f = fopen(argv[1], "r");
    graph *g = graph_parse(f);
    fclose(f);

    graph_sort_edges(g);

    int offset = 0, i = 0;
    while (argv[1][i] != '\0')
    {
        if (argv[1][i] == '/')
            offset = i + 1;
        i++;
    }

    if (!graph_validate(g))
        printf("Error in graph\n");
    else
        printf("%s, |V|=%d, |E|=%d, SD=%d\n", argv[1] + offset, g->n, g->m, dom_sum_degree_bound(g));

    graph_free(g);

    return 0;
}