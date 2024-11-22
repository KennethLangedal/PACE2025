#include "graph.h"
#include "local_search.h"
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
        printf("%s, |V|=%d, |E|=%d, SD=%d, EF=%d\n", argv[1] + offset, g->n, g->m,
               dom_sum_degree_bound(g),
               dom_efficiency_bound(g));

    local_search *ls = local_search_init(g, 0);

    local_search_explore(g, ls, 60, 1);

    if (!local_search_validate_solution(g, ls))
        printf("Error in solution\n");

    local_search_free(ls);

    graph_free(g);

    return 0;
}