#include "hypergraph.h"

#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>

#define MIN_ALLOC 8

static inline void hypergraph_parse_id(char *data, size_t *p, int *v)
{
    while (data[*p] != '\n' && (data[*p] < '0' || data[*p] > '9'))
        (*p)++;

    *v = 0;
    while (data[*p] >= '0' && data[*p] <= '9')
        *v = (*v) * 10 + data[(*p)++] - '0';
}

static inline void hypergraph_skip_line(char *data, size_t *p)
{
    while (data[*p] != '\n')
        (*p)++;
    (*p)++;
}

static inline int hypergraph_compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

void hypergraph_append_element(int *l, int *a, int **A, int v)
{
    if (*l >= *a)
    {
        *a *= 2;
        *A = realloc(*A, sizeof(int) * *a);
    }
    (*A)[(*l)++] = v;
}

hypergraph *hypergraph_init(int n, int m)
{
    hypergraph *g = malloc(sizeof(hypergraph));
    g->n = n;
    g->m = m;

    g->Vd = malloc(sizeof(int) * n);
    g->Va = malloc(sizeof(int) * n);

    g->Ed = malloc(sizeof(int) * m);
    g->Ea = malloc(sizeof(int) * m);

    g->V = malloc(sizeof(int *) * n);
    g->E = malloc(sizeof(int *) * m);

    for (int i = 0; i < n; i++)
    {
        g->Vd[i] = 0;
        g->Va[i] = MIN_ALLOC;
        g->V[i] = malloc(sizeof(int) * g->Va[i]);
    }

    for (int i = 0; i < m; i++)
    {
        g->Ed[i] = 0;
        g->Ea[i] = MIN_ALLOC;
        g->E[i] = malloc(sizeof(int) * g->Ea[i]);
    }

    return g;
}

hypergraph *hypergraph_parse_dominating_set(char *data, size_t *p)
{
    int n, m, u, v;
    hypergraph_parse_id(data, p, &n);
    hypergraph_parse_id(data, p, &m);

    hypergraph_skip_line(data, p);

    hypergraph *g = hypergraph_init(n, n);

    // Every vertex is part of their own hyperedge
    for (int i = 0; i < n; i++)
    {
        hypergraph_append_element(g->Vd + u, g->Va + u, g->V + u, u);
        hypergraph_append_element(g->Ed + u, g->Ea + u, g->E + u, u);
    }

    for (int i = 0; i < m; i++)
    {
        while (data[*p] == 'c')
            hypergraph_skip_line(data, p);

        hypergraph_parse_id(data, p, &u);
        hypergraph_parse_id(data, p, &v);

        u--;
        v--;

        hypergraph_append_element(g->Vd + u, g->Va + u, g->V + u, v);
        hypergraph_append_element(g->Ed + v, g->Ea + v, g->E + v, u);

        hypergraph_append_element(g->Vd + v, g->Va + v, g->V + v, u);
        hypergraph_append_element(g->Ed + u, g->Ea + u, g->E + u, v);

        hypergraph_skip_line(data, p);
    }

    return g;
}

hypergraph *hypergraph_parse_hitting_set(char *data, size_t *p)
{
    int n, m, v;
    hypergraph_parse_id(data, p, &n);
    hypergraph_parse_id(data, p, &m);

    hypergraph_skip_line(data, p);

    hypergraph *g = hypergraph_init(n, m);

    for (int i = 0; i < m; i++)
    {
        while (data[*p] == 'c')
            hypergraph_skip_line(data, p);

        hypergraph_parse_id(data, p, &v);
        while (v > 0)
        {
            v--;
            hypergraph_append_element(g->Vd + v, g->Va + v, g->V + v, i);
            hypergraph_append_element(g->Ed + i, g->Ea + i, g->E + i, v);

            hypergraph_parse_id(data, p, &v);
        }
        hypergraph_skip_line(data, p);
    }

    return g;
}

hypergraph *hypergraph_parse(FILE *f)
{
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = mmap(0, size, PROT_READ, MAP_PRIVATE, fileno_unlocked(f), 0);
    size_t p = 0;
    hypergraph *g = NULL;

    while (data[p] == 'c')
        hypergraph_skip_line(data, &p);

    assert(data[p] == 'p');

    p += 2;

    // Dominating Set instance
    if (data[p] == 'd' && data[p + 1] == 's')
    {
        g = hypergraph_parse_dominating_set(data, &p);
    }
    // Hitting Set instance
    else if (data[p] == 'h' && data[p + 1] == 's')
    {
        g = hypergraph_parse_hitting_set(data, &p);
    }

    munmap(data, size);

    return g;
}

void hypergraph_sort(hypergraph *g)
{
    for (int i = 0; i < g->n; i++)
        qsort(g->V[i], g->Vd[i], sizeof(int), hypergraph_compare);

    for (int i = 0; i < g->m; i++)
        qsort(g->E[i], g->Ed[i], sizeof(int), hypergraph_compare);

    for (int i = 0; i < g->n; i++)
    {
        int d = 0;
        for (int j = 0; j < g->Vd[i]; j++)
        {
            if (j == 0 || g->V[i][j] > g->V[i][j - 1])
                g->V[i][d++] = g->V[i][j];
        }
        g->Vd[i] = d;
    }
    for (int i = 0; i < g->m; i++)
    {
        int d = 0;
        for (int j = 0; j < g->Ed[i]; j++)
        {
            if (j == 0 || g->E[i][j] > g->E[i][j - 1])
                g->E[i][d++] = g->E[i][j];
        }
        g->Ed[i] = d;
    }
}

void hypergraph_free(hypergraph *g)
{
    free(g->Vd);
    free(g->Va);
    for (int i = 0; i < g->n; i++)
        free(g->V[i]);
    free(g->V);

    free(g->Ed);
    free(g->Ea);
    for (int i = 0; i < g->m; i++)
        free(g->E[i]);
    free(g->E);

    free(g);
}

int hypergraph_validate(hypergraph *g)
{
    for (int i = 0; i < g->n; i++)
    {
        for (int j = 0; j < g->Vd[i]; j++)
        {
            int e = g->V[i][j];
            if (e < 0 || e >= g->m || g->Va[i] < g->Vd[i])
                return 0;
            if (j > 0 && e <= g->V[i][j - 1])
                return 0;

            int *p = bsearch(&i, g->E[e], g->Ed[e], sizeof(int), hypergraph_compare);
            if (p == NULL || *p != i)
                return 0;
        }
    }

    for (int i = 0; i < g->m; i++)
    {
        for (int j = 0; j < g->Ed[i]; j++)
        {
            int v = g->E[i][j];
            if (v < 0 || v >= g->n || g->Ea[i] < g->Ed[i])
                return 0;
            if (j > 0 && v <= g->E[i][j - 1])
                return 0;

            int *p = bsearch(&i, g->V[v], g->Vd[v], sizeof(int), hypergraph_compare);
            if (p == NULL || *p != i)
                return 0;
        }
    }

    return 1;
}

void hypergraph_remove_vertex(hypergraph *g, int u)
{
    for (int i = 0; i < g->Vd[u]; i++)
    {
        int e = g->V[u][i];

        int p = -1;
        for (int j = 0; j < g->Ed[e]; j++)
        {
            if (g->E[e][j] == u)
            {
                p = j;
                break;
            }
        }
        assert(p >= 0);
        for (int j = p; j < g->Ed[e] - 1; j++)
        {
            g->E[e][j] = g->E[e][j + 1];
        }
        g->Ed[e]--;
    }

    g->Vd[u] = 0;
}

void hypergraph_remove_edge(hypergraph *g, int e)
{
    for (int i = 0; i < g->Ed[e]; i++)
    {
        int v = g->E[e][i];

        int p = -1;
        for (int j = 0; j < g->Vd[v]; j++)
        {
            if (g->V[v][j] == e)
            {
                p = j;
                break;
            }
        }
        assert(p >= 0);
        for (int j = p; j < g->Vd[v] - 1; j++)
        {
            g->V[v][j] = g->V[v][j + 1];
        }
        g->Vd[v]--;
    }

    g->Ed[e] = 0;
}