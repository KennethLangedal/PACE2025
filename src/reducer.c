#include "reducer.h"

#include <stdlib.h>

reducer *reducer_init(graph *g, int n_rules, ...)
{
    va_list args;
    va_start(args, n_rules);

    reducer *r = malloc(sizeof(reducer));
    r->n_rules = n_rules;
    r->_a = g->_a;

    r->rules = malloc(sizeof(reduction) * r->n_rules);

    r->queue_count = malloc(sizeof(int) * r->n_rules);
    r->queue = malloc(sizeof(int *) * r->n_rules);
    r->in_queue = malloc(sizeof(int *) * r->n_rules);

    for (int i = 0; i < r->n_rules; i++)
    {
        r->queue_count[i] = g->n;
        r->queue[i] = malloc(sizeof(int) * r->_a);
        r->in_queue[i] = malloc(sizeof(int) * r->_a);

        // Perhaps make filling the queues optional..
        for (int j = 0; j < g->n; j++)
        {
            r->queue[i][j] = j;
            r->in_queue[i][j] = 1;
        }
        for (int j = g->n; j < r->_a; j++)
        {
            r->in_queue[i][j] = 0;
        }

        r->rules[i] = va_arg(args, reduction);
    }

    r->data = reduction_data_init(g);

    va_end(args);

    return r;
}

void reducer_free(reducer *r)
{
    free(r->queue_count);

    for (int i = 0; i < r->n_rules; i++)
    {
        free(r->queue[i]);
        free(r->in_queue[i]);
    }

    free(r->queue);
    free(r->in_queue);

    free(r->rules);
    reduction_data_free(r->data);

    free(r);
}

reduction_log *reducer_init_reduction_log()
{
    reduction_log *l = malloc(sizeof(reduction_log));

    l->n = 0;
    l->offset = 0ll;

    l->_a = (1 << 10);
    l->log_data = malloc(sizeof(reconstruction_data) * l->_a);
    l->log_rule = malloc(sizeof(reduction) * l->_a);

    return l;
}

void reducer_free_reduction_log(reduction_log *l)
{
    for (int i = 0; i < l->n; i++)
    {
        l->log_rule[i].clean(&l->log_data[i]);
    }

    free(l->log_data);
    free(l->log_rule);

    free(l);
}

reduction_log *reducer_reduce(reducer *r, graph *g)
{
    reduction_log *l = reducer_init_reduction_log();
    reducer_reduce_continue(r, g, l);
    return l;
}

void reducer_update(reducer *r, graph *g, reduction_log *l)
{
    l->n++;
    if (l->n == l->_a)
    {
        l->_a *= 2;
        l->log_data = realloc(l->log_data, sizeof(reconstruction_data) * l->_a);
        l->log_rule = realloc(l->log_rule, sizeof(reduction) * l->_a);
    }

    while (g->_a > r->_a)
    {
        r->_a *= 2;
        for (int i = 0; i < r->n_rules; i++)
        {
            r->queue[i] = realloc(r->queue[i], sizeof(int) * r->_a);
            r->in_queue[i] = realloc(r->in_queue[i], sizeof(int) * r->_a);
        }
    }

    reduction_data *rd = r->data;
    l->offset += l->log_data[l->n - 1].offset;

    while (g->_a > rd->_a)
        reduction_data_increase(rd);

    for (int i = 0; i < rd->n_changed; i++)
    {
        int u = rd->changed[i];
        for (int j = 0; j < r->n_rules; j++)
        {
            if (!r->in_queue[j][u])
            {
                r->queue[j][r->queue_count[j]++] = u;
                r->in_queue[j][u] = 1;
            }
        }
    }
}

void reducer_reduce_continue(reducer *r, graph *g, reduction_log *l)
{
    int rule = 0;
    while (rule < r->n_rules)
    {
        if (r->queue_count[rule] == 0)
        {
            rule++;
            continue;
        }

        if (r->rules[rule].global)
        {
            // Clear the queue
            for (int i = 0; i < r->queue_count[rule]; i++)
                r->in_queue[r->queue[rule][i]] = 0;
            r->queue_count[rule] = 0;

            int res = r->rules[rule].reduce(g, 0, r->data, l->log_data + l->n);
            if (res)
            {
                l->log_rule[l->n] = r->rules[rule];
                reducer_update(r, g, l);
                rule = 0;
            }
        }
        else
        {
            // Pop from queue
            int u = r->queue[rule][--r->queue_count[rule]];
            r->in_queue[rule][u] = 0;

            if (!g->A[u])
                continue;

            int res = r->rules[rule].reduce(g, u, r->data, l->log_data + l->n);
            if (res)
            {
                l->log_rule[l->n] = r->rules[rule];
                reducer_update(r, g, l);
                rule = 0;
            }
        }
    }
}

void reducer_include_vertex(reducer *r, graph *g, reduction_log *l, int u)
{
    // TODO
}

void reducer_exclude_vertex(reducer *r, graph *g, reduction_log *l, int u)
{
    // TODO
}

void reducer_restore_graph(graph *g, reduction_log *l, int t)
{
    while (l->n > t)
    {
        l->n--;
        l->offset -= l->log_data[l->n].offset;
        l->log_rule[l->n].restore(g, &l->log_data[l->n]);
        l->log_rule[l->n].clean(&l->log_data[l->n]);
    }
}

void reducer_lift_solution(reduction_log *l, int *I)
{
    for (int i = l->n - 1; i >= 0; i--)
    {
        l->log_rule[i].lift_solution(I, &l->log_data[i]);
    }
}