#include "hypergraph.h"
#include "hs_reducer.h"

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

hs_reducer *hs_reducer_init(hypergraph *g, int n_rules, ...)
{
    va_list args;
    va_start(args, n_rules);

    hs_reducer *r = malloc(sizeof(hs_reducer));
    r->n_rules = n_rules;

    r->Rule = malloc(sizeof(hs_reduction) * r->n_rules);

    r->Queue_count   = malloc(sizeof(int) * (r->n_rules));
    r->Queue_count_E = malloc(sizeof(int) * (r->n_rules));
    r->Queues   = malloc(sizeof(int *) * (r->n_rules));
    r->Queues_E = malloc(sizeof(int *) * (r->n_rules));
    r->In_queues = malloc(sizeof(int *) * (r->n_rules));
    r->In_queues_E = malloc(sizeof(int *) * (r->n_rules));

    r->fast_set = malloc(sizeof(int) * (g->n));
    r->fs_count = 0;

    for (int i = 0; i < r->n_rules; i++)
    {
        r->Queue_count[i] = g->n;
        r->Queue_count_E[i] = g->m;
        r->Queues[i] = malloc(sizeof(int) * g->n);
        r->Queues_E[i] = malloc(sizeof(int) * g->m);
        r->In_queues[i] = malloc(sizeof(int) * g->n);
        r->In_queues_E[i] = malloc(sizeof(int) * g->m);

        // Perhaps make filling the queues optional..
        for (int j = 0; j < g->n; j++)
        {
            r->Queues[i][j] = j;
            r->In_queues[i][j] = 1;
        }
        for (int j = 0; j < g->m; j++)
        {
            r->Queues_E[i][j] = j;
            r->In_queues_E[i][j] = 1;
        }
    }

    r->c = malloc(sizeof(hs_change_list));
    r->c->n = 0;
    r->c->m = 0;
    r->c->V = malloc(sizeof(int) * g->n);
    r->c->E = malloc(sizeof(int) * g->m);
    r->c->in_V = malloc(sizeof(int) * g->n);
    r->c->in_E = malloc(sizeof(int) * g->m);

    for (int i = 0; i < g->n; i++)
        r->c->in_V[i] = 0;

    for (int i = 0; i < g->m; i++)
        r->c->in_E[i] = 0;

    for (int i = 0; i < r->n_rules; i++)
        r->Rule[i] = va_arg(args, hs_reduction);

    va_end(args);

    return r;
}

void hs_reducer_free(hs_reducer *r)
{
    free(r->Queue_count);

    for (int i = 0; i < r->n_rules; i++)
    {
        free(r->Queues[i]);
        free(r->In_queues[i]);
    }

    free(r->Queues);
    free(r->In_queues);

    free(r->Rule);
    free(r->fast_set);

    free(r);
}

void hs_reducer_queue_changed(hypergraph *g, hs_reducer *r)
{
    for (int i = 0; i < r->c->n; i++)
    {
        int v = r->c->V[i];
        r->c->in_V[v] = 0;

        for (int j = 0; j < r->n_rules; j++)
        {
            if (!r->In_queues[j][v])
            {
                r->Queues[j][r->Queue_count[j]++] = v;
                r->In_queues[j][v] = 1;
            }
        }
    }
    r->c->n = 0;

    for (int i = 0; i < r->c->m; i++)
    {
        int e = r->c->E[i];
        r->c->in_E[e] = 0;
        for (int j = 0; j < r->n_rules; j++)
        {
            if (!r->In_queues_E[j][e])
            {
                r->Queues_E[j][r->Queue_count_E[j]++] = e;
                r->In_queues_E[j][e] = 1;
            }
        }
    }
    r->c->m = 0;
}

void hs_reducer_queue_up_neighbors_v(hypergraph *g, int u, hs_change_list *c)
{
    c->n = 0;
    c->m = 0;
    for (int i = 0; i < g->Vd[u]; i++)
    {
        int e = g->V[u][i];
        c->E[c->m++] = e;
        c->in_E[e] = 1;
        for (int j = 0; j < g->Ed[e]; j++)
        {
            int v = g->E[e][j];
            c->V[c->n++] = v;
            c->in_V[v] = 1;
        }
    }
}

void hs_reducer_queue_up_neighbors_e(hypergraph *g, int e, hs_change_list *c)
{
    c->n = 0;
    c->m = 0;
    for (int j = 0; j < g->Ed[e]; j++)
    {
        int v = g->E[e][j];
        c->V[c->n++] = v;
        c->in_V[v] = 1;
        for (int i = 0; i < g->Vd[v]; i++)
        {
            int f = g->V[v][i];
            c->E[c->m++] = f;
            c->in_E[f] = 1;
        }
    }
}

int hs_reducer_apply_reduction(hypergraph *g, int u, int apply_on_edges, hs_reduction rule, hs_reducer *r)
{
    int res = rule.reduce(g, u, apply_on_edges, r->c, r->fast_set, r->fs_count);
    r->fs_count++;

    if (r->fs_count == (1 << 30))
        hs_reducer_reset_fast_set(g, r);

    if (!res)
        return 0;

    hs_reducer_queue_changed(g, r);

    return 1;
}

// void hs_reducer_reduce(hs_reducer *r, hypergraph *g, double tl)
void hs_reducer_reduce(hs_reducer *r, hypergraph *g)
{
    int rule = 0;
    // double t0 = get_wtime();
    int apply_on_edges = 0;
    // while (rule < r->n_rules && get_wtime() - t0 < tl)
    while (rule < r->n_rules)
    {
        if (apply_on_edges == 0 && r->Queue_count[rule] == 0)
        {
            apply_on_edges = 1;
            continue;
        }
        if (apply_on_edges == 1 && r->Queue_count_E[rule] == 0)
        {
            apply_on_edges = 0;
            rule++;
            continue;
        }

        int next = 0;
        if (r->Rule[rule].global)
        {
            while (r->Queue_count[rule] > 0)
            {
                int v = r->Queues[rule][--r->Queue_count[rule]];
                r->In_queues[rule][v] = 0;
            }
            while (r->Queue_count_E[rule] > 0)
            {
                int e = r->Queues_E[rule][--r->Queue_count_E[rule]];
                r->In_queues_E[rule][e] = 0;
            }
        }
        else
        {
            if (apply_on_edges == 0)
            {
                next = r->Queues[rule][--r->Queue_count[rule]];
                r->In_queues[rule][next] = 0;
            }
            else
            {
                next = r->Queues_E[rule][--r->Queue_count_E[rule]];
                r->In_queues_E[rule][next] = 0;
            }
        }

        int res = hs_reducer_apply_reduction(g, next, apply_on_edges, r->Rule[rule], r);

        if (res)
            rule = 0;
    }
}

void hs_reducer_reset_fast_set(hypergraph *g, hs_reducer *r)
{
    r->fs_count = 1;
    for (int i = 0; i < g->n; i++)
        r->fast_set[i] = 0;
}
