#include "mwis_reductions.h"

#include <stdlib.h>

void red_data_resize(red_data *rd)
{
    rd->_a *= 2;

    for (int i = 0; i < N_BUFFERS; i++)
    {
        rd->buffers[i] = realloc(rd->buffers[i], sizeof(int) * rd->_a);
        rd->fast_sets[i] = realloc(rd->fast_sets[i], sizeof(int) * rd->_a);
    }

    rd->changed = realloc(rd->changed, sizeof(int) * rd->_a);
}

red_data *red_data_init(graph *g)
{
    red_data *rd = malloc(sizeof(red_data));

    rd->_a = g->_a;

    rd->buffers = malloc(sizeof(int *) * N_BUFFERS);
    rd->fast_sets = malloc(sizeof(int *) * N_BUFFERS);

    for (int i = 0; i < N_BUFFERS; i++)
    {
        rd->buffers[i] = malloc(sizeof(int) * rd->_a);
        rd->fast_sets[i] = malloc(sizeof(int) * rd->_a);
    }

    rd->n = 0;
    rd->changed = malloc(sizeof(int) * rd->_a);

    rd->offset = 0;

    return rd;
}

void red_data_free(red_data *rd)
{
    for (int i = 0; i < N_BUFFERS; i++)
    {
        free(rd->buffers[i]);
        free(rd->fast_sets[i]);
    }

    free(rd->buffers);
    free(rd->fast_sets);

    free(rd->changed);

    free(rd);
}

// Helper functions

void queue_neighborhood(graph *g, int u, red_data *rd)
{
    for (int i = 0; i < g->D[u]; i++)
    {
        int v = g->V[u][i];
        if (!g->A[v])
            continue;
        rd->changed[rd->n++] = v;
    }
}

// Degree Zero

int degree_zero_reduction(graph *g, int u, red_data *rd, void **rec)
{
    assert(g->A[u]);

    if (g->D[u] == 0)
    {
        graph_deactivate_vertex(g, u);

        rd->offset += g->W[u];
        rd->n = 0;

        int *v = malloc(sizeof(int));
        *v = u;
        *rec = (void *)v;

        return 1;
    }

    return 0;
}

void degree_zero_reconstruct_graph(graph *g, red_data *rd, void *rec)
{
    int u = *(int *)rec;
    assert(!g->A[u]);

    graph_activate_vertex(g, u);
    rd->offset -= g->W[u];
}

void degree_zero_reconstruct_solution(int *I, void *rec)
{
    int u = *(int *)rec;
    I[u] = 1;
}

void degree_zero_free(void *rec)
{
    free(rec);
}

// Degree One

typedef struct
{
    int u, v, f;
} degree_one;

int degree_one_reduction(graph *g, int u, red_data *rd, void **rec)
{
    assert(g->A[u]);

    if (g->D[u] == 1)
    {
        int v = g->V[u][0];

        rd->offset += g->W[u];

        rd->n = 0;
        queue_neighborhood(g, v, rd);

        degree_one *data = malloc(sizeof(degree_one));
        *data = (degree_one){.u = u, .v = v, .f = 0};

        if (g->W[u] >= g->W[v])
        {
            graph_deactivate_neighborhood(g, u);
        }
        else
        {
            graph_deactivate_vertex(g, u);
            g->W[v] -= g->W[u];
            data->f = 1;
        }

        *rec = (void *)data;

        return 1;
    }

    return 0;
}

void degree_one_reconstruct_graph(graph *g, red_data *rd, void *rec)
{
    degree_one *data = (degree_one *)rec;
    int u = data->u, v = data->v;

    rd->offset -= g->W[u];

    if (data->f)
    {
        assert(!g->A[u] && g->A[v]);
        graph_activate_vertex(g, u);
        g->W[v] += g->W[u];
    }
    else
    {
        assert(!g->A[u] && !g->A[v]);
        graph_activate_neighborhood(g, u);
    }
}

void degree_one_reconstruct_solution(int *I, void *rec)
{
    degree_one *data = (degree_one *)rec;
    int u = data->u, v = data->v;

    if (data->f)
    {
        if (!I[v])
            I[u] = 1;
    }
    else
    {
        I[u] = 1;
    }
}

void degree_one_free(void *rec)
{
    free(rec);
}