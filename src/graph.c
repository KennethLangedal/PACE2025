#include "graph.h"

#include <stdlib.h>
#include <sys/mman.h>

static inline void parse_id(char *data, size_t *p, long long *v)
{
    while (data[*p] < '0' || data[*p] > '9')
        (*p)++;

    *v = 0;
    while (data[*p] >= '0' && data[*p] <= '9')
        *v = (*v) * 10 + data[(*p)++] - '0';
}

static inline int compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

graph *graph_parse(FILE *f)
{
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = mmap(0, size, PROT_READ, MAP_PRIVATE, fileno_unlocked(f), 0);
    size_t p = 0;

    long long n, m;
    parse_id(data, &p, &n);
    parse_id(data, &p, &m);

    long long *X = malloc(sizeof(long long) * m);
    long long *Y = malloc(sizeof(long long) * m);

    int *D = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++)
        D[i] = 0;

    for (int i = 0; i < m; i++)
    {
        parse_id(data, &p, X + i);
        parse_id(data, &p, Y + i);

        D[X[i] - 1]++;
        D[Y[i] - 1]++;
    }

    int *V = malloc(sizeof(int) * (n + 1));
    int *E = malloc(sizeof(int) * (m * 2));

    V[0] = 0;
    for (int i = 0; i < n; i++)
    {
        V[i + 1] = V[i] + D[i];
        D[i] = 0;
    }

    for (int i = 0; i < m; i++)
    {
        int u = X[i] - 1, v = Y[i] - 1;

        E[V[u] + D[u]] = v;
        E[V[v] + D[v]] = u;

        D[u]++;
        D[v]++;
    }

    munmap(data, size);

    free(X);
    free(Y);
    free(D);

    graph *g = malloc(sizeof(graph));
    *g = (graph){.n = n, .m = m, .V = V, .E = E};

    return g;
}

void graph_sort_edges(graph *g)
{
    for (int i = 0; i < g->n; i++)
        qsort(g->E + g->V[i], g->V[i + 1] - g->V[i], sizeof(int), compare);

    int ei;
    for (int i = 0; i < g->n; i++)
    {
        int s = g->V[i];
        g->V[i] = ei;
        for (int j = s; j < g->V[i + 1]; j++)
            if (g->E[j] != i && (j == s || g->E[j] > g->E[ei - 1]))
                g->E[ei++] = g->E[j];
    }
    g->V[g->n] = ei;

    if (ei < g->m * 2)
    {
        g->m = ei / 2;
        g->E = realloc(g->E, sizeof(int) * g->m * 2);
    }
}

void graph_free(graph *g)
{
    free(g->V);
    free(g->E);
    free(g);
}

int graph_validate(graph *g)
{
    int m = 0;
    for (int u = 0; u < g->n; u++)
    {
        if (g->V[u + 1] - g->V[u] < 0)
            return 0;

        m += g->V[u + 1] - g->V[u];

        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            if (i < 0 || i >= g->V[g->n])
                return 0;

            int v = g->E[i];
            if (v < 0 || v >= g->n || v == u || (i > g->V[u] && v <= g->E[i - 1]))
                return 0;

            if (bsearch(&u, g->E + g->V[v], g->V[v + 1] - g->V[v], sizeof(int), compare) == NULL)
                return 0;
        }
    }

    if (m != g->V[g->n] || m / 2 != g->m)
        return 0;

    return 1;
}