#include "hypergraph.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#define MIN_ALLOC 8

static inline void skip_comments(FILE *f)
{
    int c = fgetc_unlocked(f);
    while (c == 'c')
    {
        while (c != '\n')
            c = fgetc_unlocked(f);
        c = fgetc_unlocked(f);
    }
    ungetc(c, f);
}

static inline void skip_line(FILE *f)
{
    int c = fgetc_unlocked(f);
    while (c != '\n')
        c = fgetc_unlocked(f);
}

static inline void parse_unsigned_int(FILE *f, int *v)
{
    int c = fgetc_unlocked(f);
    while ((c < '0' || c > '9') && c != '\n')
        c = fgetc_unlocked(f);

    *v = -1;
    if (c == '\n')
    {
        ungetc(c, f);
        return;
    }

    *v = 0;
    while (c >= '0' && c <= '9')
    {
        *v = (*v * 10) + (c - '0');
        c = fgetc_unlocked(f);
    }
    ungetc(c, f);
}

static inline int hypergraph_compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

static inline int lower_bound(const int *A, int n, int x)
{
    const int *s = A;
    while (n > 1)
    {
        int h = n / 2;
        s += (s[h - 1] < x) * h;
        n -= h;
    }
    s += (n == 1 && s[0] < x);
    return s - A;
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

hypergraph *hypergraph_parse_dominating_set(FILE *f)
{
    int n, m, u, v;
    parse_unsigned_int(f, &n);
    parse_unsigned_int(f, &m);

    hypergraph *g = hypergraph_init(n, n);

    // Every vertex is part of their own hyperedge
    for (int u = 0; u < n; u++)
    {
        hypergraph_append_element(g->Vd + u, g->Va + u, g->V + u, u);
        hypergraph_append_element(g->Ed + u, g->Ea + u, g->E + u, u);
    }

    for (int i = 0; i < m; i++)
    {
        skip_line(f);
        skip_comments(f);

        parse_unsigned_int(f, &u);
        parse_unsigned_int(f, &v);

        u--;
        v--;

        hypergraph_append_element(g->Vd + u, g->Va + u, g->V + u, v);
        hypergraph_append_element(g->Ed + v, g->Ea + v, g->E + v, u);

        hypergraph_append_element(g->Vd + v, g->Va + v, g->V + v, u);
        hypergraph_append_element(g->Ed + u, g->Ea + u, g->E + u, v);
    }

    return g;
}

hypergraph *hypergraph_parse_hitting_set(FILE *f)
{
    int n, m, v;
    parse_unsigned_int(f, &n);
    parse_unsigned_int(f, &m);

    hypergraph *g = hypergraph_init(n, m);

    for (int i = 0; i < m; i++)
    {
        skip_line(f);
        skip_comments(f);

        parse_unsigned_int(f, &v);
        while (v > 0)
        {
            v--;
            hypergraph_append_element(g->Vd + v, g->Va + v, g->V + v, i);
            hypergraph_append_element(g->Ed + i, g->Ea + i, g->E + i, v);

            parse_unsigned_int(f, &v);
        }
    }

    return g;
}

hypergraph *hypergraph_parse(FILE *f)
{
    hypergraph *g = NULL;

    skip_comments(f);

    int c = fgetc_unlocked(f);
    c = fgetc_unlocked(f);
    c = fgetc_unlocked(f);

    // Dominating Set instance
    if (c == 'd')
    {
        g = hypergraph_parse_dominating_set(f);
    }
    // Hitting Set instance
    else if (c == 'h')
    {
        g = hypergraph_parse_hitting_set(f);
    }

    return g;
}

hypergraph *hypergraph_copy(hypergraph *g)
{
    hypergraph *c = malloc(sizeof(hypergraph));

    *c = (hypergraph){.n = g->n, .m = g->m};

    c->Vd = malloc(sizeof(int *) * c->n);
    c->Va = malloc(sizeof(int *) * c->n);
    c->V = malloc(sizeof(int *) * c->n);

    for (int i = 0; i < c->n; i++)
    {
        c->Vd[i] = g->Vd[i];
        c->Va[i] = g->Va[i];
        c->V[i] = malloc(sizeof(int) * c->Va[i]);

        for (int j = 0; j < c->Vd[i]; j++)
            c->V[i][j] = g->V[i][j];
    }

    c->Ed = malloc(sizeof(int *) * c->m);
    c->Ea = malloc(sizeof(int *) * c->m);
    c->E = malloc(sizeof(int *) * c->m);

    for (int i = 0; i < c->m; i++)
    {
        c->Ed[i] = g->Ed[i];
        c->Ea[i] = g->Ea[i];
        c->E[i] = malloc(sizeof(int) * c->Ea[i]);

        for (int j = 0; j < c->Ed[i]; j++)
            c->E[i][j] = g->E[i][j];
    }

    return c;
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
            if (j == 0 || g->V[i][j] > g->V[i][d - 1])
                g->V[i][d++] = g->V[i][j];
        }
        g->Vd[i] = d;
    }
    for (int i = 0; i < g->m; i++)
    {
        int d = 0;
        for (int j = 0; j < g->Ed[i]; j++)
        {
            if (j == 0 || g->E[i][j] > g->E[i][d - 1])
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

            int p = lower_bound(g->E[e], g->Ed[e], i);
            if (p == g->Ed[e] || g->E[e][p] != i)
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

            int p = lower_bound(g->V[v], g->Vd[v], i);
            if (p == g->Vd[v] || g->V[v][p] != i)
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

        int p = lower_bound(g->E[e], g->Ed[e], u);
        assert(p < g->Ed[e] && g->E[e][p] == u);
        memmove(g->E[e] + p, g->E[e] + p + 1, sizeof(int) * (g->Ed[e] - p - 1));
        g->Ed[e]--;
    }

    g->Vd[u] = 0;
}

void hypergraph_remove_edge(hypergraph *g, int e)
{
    for (int i = 0; i < g->Ed[e]; i++)
    {
        int v = g->E[e][i];

        int p = lower_bound(g->V[v], g->Vd[v], e);
        assert(p < g->Vd[v] && g->V[v][p] == e);
        memmove(g->V[v] + p, g->V[v] + p + 1, sizeof(int) * (g->Vd[v] - p - 1));
        g->Vd[v]--;
    }
    g->Ed[e] = 0;
    g->Ea[e] = MIN_ALLOC;
    g->E[e] = realloc(g->E[e], sizeof(int) * g->Ea[e]);
}

void hypergraph_include_vertex(hypergraph *g, int u)
{
    if (g->Vd[u] == 0)
        return;

    int e = g->V[u][0];
    while (g->Vd[u] > 0)
        hypergraph_remove_edge(g, g->V[u][0]);

    g->Ed[e] = 1;
    g->E[e][0] = u;
    g->Vd[u] = 1;
    g->V[u][0] = e;
    g->Va[u] = MIN_ALLOC;
    g->V[u] = realloc(g->V[u], sizeof(int) * g->Va[u]);
}