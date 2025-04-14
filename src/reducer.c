#include "reducer.h"

#include <stdlib.h>

reducer *reducer_init(graph *g, int n_rules, ...)
{
    va_list args;

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
        r->queue[i] = malloc(sizeof(int) * g->_a);
        r->in_queue[i] = malloc(sizeof(int) * g->_a);

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
    free(r->data);
}

reduction_log *reducer_init_reduction_log()
{
    reduction_log *l = malloc(sizeof(reduction_log));

    l->n = 0;
    l->offset = 0ll;

    l->_a = (1 << 10);
    l->log_data = malloc(sizeof(void *) * l->_a);
    l->log_rule = malloc(sizeof(reduction) * l->_a);

    return l;
}

void reducer_free_reduction_log(reduction_log *l)
{
    for (int i = 0; i < l->n; i++)
    {
        l->log_rule[i].clean(l->log_data[i]);
    }

    free(l->log_data);
    free(l->log_rule);
}

reduction_log *reducer_reduce(reducer *r, graph *g)
{
    reduction_log *l = reducer_init_reduction_log();
    reducer_reduce_continue(r, g, l);
    return l;
}

void reducer_reduce_continue(reducer *r, graph *g, reduction_log *l)
{
    int rule = 0;
    while (rule < r->n_rules)
    {
        if (r->queue_count[rule] == 0)
            rule++;
        
        
    }
}

void reducer_include_vertex(reducer *r, graph *g, reduction_log *l, int u);

void reducer_exclude_vertex(reducer *r, graph *g, reduction_log *l, int u);

void reducer_restore_graph(graph *g, reduction_log *l, int t);

void reducer_lift_solution(reduction_log *l, int *I);