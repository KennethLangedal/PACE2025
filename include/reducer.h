#pragma once

#include "graph.h"
#include "reductions.h"

#include <stdarg.h>

typedef struct
{
    int n_rules, _a;
    reduction *rules;

    int *queue_count, **queue, **in_queue;

    reduction_data *data;
} reducer;

typedef struct
{
    int n, _a;
    void **log_data;
    reduction *log_rule;

    long long offset;
} reduction_log;

reducer *reducer_init(graph *g, int n_rules, ...);

void reducer_free(reducer *r);

void reducer_free_reduction_log(reduction_log *l);

reduction_log *reducer_reduce(reducer *r, graph *g);

void reducer_reduce_continue(reducer *r, graph *g, reduction_log *l);

void reducer_include_vertex(reducer *r, graph *g, reduction_log *l, int u);

void reducer_exclude_vertex(reducer *r, graph *g, reduction_log *l, int u);

void reducer_restore_graph(graph *g, reduction_log *l, int t);

void reducer_lift_solution(reduction_log *l, int *I);