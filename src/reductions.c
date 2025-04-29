#include "reductions.h"

#include <assert.h>
#include <stdlib.h>

// Test if A is a subset of B
static inline int test_subset(const int *A, int a, const int *B, int b)
{
    if (b < a)
        return 0;

    int i = 0, j = 0;
    while (i < a && j < b)
    {
        if (A[i] > B[j])
        {
            j++;
        }
        else if (A[i] == B[j])
        {
            i++;
            j++;
        }
        else
        {
            return 0;
        }
    }

    return i == a;
}

int reduction_vertex_domination(hypergraph *g)
{
    int r = 0;
    for (int i = 0; i < g->n; i++)
    {
        int md = -1;
        for (int j = 0; j < g->Vd[i]; j++)
        {
            int e = g->V[i][j];
            if (md < 0 || g->Ed[e] < g->Ed[md])
                md = e;
        }
        if (md < 0)
            continue;

        for (int j = 0; j < g->Ed[md]; j++)
        {
            int v = g->E[md][j];
            if (v == i)
                continue;

            if (test_subset(g->V[i], g->Vd[i], g->V[v], g->Vd[v]))
            {
                r++;
                hypergraph_remove_vertex(g, i);
                break;
            }
        }
    }
    return r;
}

int reduction_edge_domination(hypergraph *g)
{
    int r = 0;
    for (int i = 0; i < g->m; i++)
    {
        int md = -1;
        for (int j = 0; j < g->Ed[i]; j++)
        {
            int v = g->E[i][j];
            if (md < 0 || g->Vd[v] < g->Vd[md])
                md = v;
        }
        if (md < 0)
            continue;

        for (int j = 0; j < g->Vd[md]; j++)
        {
            int e = g->V[md][j];
            if (e == i)
                continue;

            if (test_subset(g->E[i], g->Ed[i], g->E[e], g->Ed[e]))
            {
                r++;
                hypergraph_remove_edge(g, e);
            }
        }
    }
    return r;
}

graph *reduction_hitting_set_to_mwis(hypergraph *hg)
{
    graph *g = graph_init();

    for (int i = 0; i < hg->n; i++)
    {
        graph_add_vertex(g, 1);
    }

    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] == 0)
            continue;
        else if (hg->Ed[i] == 2)
        {
            graph_add_edge(g, hg->E[i][0], hg->E[i][1]);
            continue;
        }
        int s = g->n;
        for (int j = 0; j < hg->Ed[i]; j++)
        {
            graph_add_vertex(g, 1000);
        }
        // Make clique
        for (int j = s; j < g->n; j++)
        {
            for (int k = j + 1; k < g->n; k++)
            {
                graph_add_edge(g, j, k);
            }
        }
        // Connect to top layer
        for (int j = 0; j < hg->Ed[i]; j++)
        {
            int v = hg->E[i][j];
            graph_add_edge(g, v, s + j);
        }
    }

    graph_sort_edges(g);

    return g;
}

reduction_data *reduction_data_init(graph *g)
{
    reduction_data *rd = malloc(sizeof(reduction_data));

    *rd = (reduction_data){._a = g->_a, .t = 1, .n_changed = 0};

    rd->buffers = malloc(sizeof(int *) * N_BUFFERS);
    rd->fast_sets = malloc(sizeof(int *) * N_BUFFERS);

    rd->changed = malloc(sizeof(int) * rd->_a);

    for (int i = 0; i < N_BUFFERS; i++)
    {
        rd->buffers[i] = malloc(sizeof(int) * rd->_a);
        rd->fast_sets[i] = malloc(sizeof(int) * rd->_a);

        for (int j = 0; j < rd->_a; j++)
        {
            rd->fast_sets[i][j] = 0;
        }
    }

    return rd;
}

void reduction_data_increase(reduction_data *rd)
{
    rd->_a *= 2;
    rd->t = 1;

    rd->changed = realloc(rd->changed, sizeof(int) * rd->_a);

    for (int i = 0; i < N_BUFFERS; i++)
    {
        rd->buffers[i] = realloc(rd->buffers[i], sizeof(int) * rd->_a);
        rd->fast_sets[i] = realloc(rd->fast_sets[i], sizeof(int) * rd->_a);

        for (int j = 0; j < rd->_a; j++)
        {
            rd->fast_sets[i][j] = 0;
        }
    }
}

void reduction_data_free(reduction_data *rd)
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

void queue_neighborhood(graph *g, int u, reduction_data *rd)
{
    for (int i = 0; i < g->D[u]; i++)
    {
        int v = g->V[u][i];
        if (!g->A[v])
            continue;
        rd->changed[rd->n_changed++] = v;
    }
}

int degree_zero_reduce(graph *g, int u, reduction_data *rd, reconstruction_data *rc)
{
    assert(g->A[u]);

    if (g->D[u] == 0)
    {
        graph_deactivate_vertex(g, u);

        rc->offset = g->W[u];
        rc->u = u;

        rd->n_changed = 0;
        return 1;
    }

    return 0;
}

void degree_zero_reconstruct_graph(graph *g, reconstruction_data *rc)
{
    assert(!g->A[rc->u]);

    graph_activate_vertex(g, rc->u);
}

void degree_zero_reconstruct_solution(int *I, reconstruction_data *rc)
{
    I[rc->u] = 1;
}

void degree_zero_free(reconstruction_data *rc)
{
}

int degree_one_reduce(graph *g, int u, reduction_data *rd, reconstruction_data *rc)
{
    assert(g->A[u]);

    if (g->D[u] == 1)
    {
        int v = g->V[u][0];

        rc->offset = g->W[u];

        rd->n_changed = 0;
        queue_neighborhood(g, v, rd);

        rc->u = u;
        rc->v = v;
        rc->x = 0;

        if (g->W[u] >= g->W[v])
        {
            graph_deactivate_neighborhood(g, u);
        }
        else
        {
            graph_deactivate_vertex(g, u);
            g->W[v] -= g->W[u];
            rc->x = 1;
        }

        return 1;
    }

    return 0;
}

void degree_one_reconstruct_graph(graph *g, reconstruction_data *rc)
{
    int u = rc->u, v = rc->v;

    if (rc->x)
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

void degree_one_reconstruct_solution(int *I, reconstruction_data *rc)
{
    int u = rc->u, v = rc->v;

    if (rc->x)
    {
        if (!I[v])
            I[u] = 1;
    }
    else
    {
        I[v] = 0;
        I[u] = 1;
    }
}

void degree_one_free(reconstruction_data *rc)
{
}

int domination_reduce(graph *g, int u, reduction_data *rd, reconstruction_data *rc)
{
    assert(g->A[u]);

    int md = -1;
    for (int i = 0; i < g->D[u]; i++)
    {
        int v = g->V[u][i];
        if (md < 0 || g->D[md] > g->D[v])
            md = v;
    }

    for (int i = 0; i < g->D[md]; i++)
    {
        int v = g->V[md][i];
        if (v != u && g->W[v] <= g->W[u] && test_subset(g->V[u], g->D[u], g->V[v], g->D[v]))
        {
            graph_deactivate_vertex(g, v);

            rd->n_changed = 0;
            queue_neighborhood(g, v, rd);

            rc->u = v;
            rc->offset = 0;

            return 1;
        }
    }
    return 0;
}

void domination_reconstruct_graph(graph *g, reconstruction_data *rc)
{
    int u = rc->u;
    assert(!g->A[u]);

    graph_activate_vertex(g, u);
}

void domination_reconstruct_solution(int *I, reconstruction_data *rc)
{
    I[rc->u] = 0;
}

void domination_free(reconstruction_data *rc)
{
}