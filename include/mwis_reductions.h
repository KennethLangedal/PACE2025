#pragma once

#include "graph.h"

#include <stdarg.h>

#define N_BUFFERS 3

typedef struct
{
    int _a;

    // Buffers
    int **buffers;

    // Fast sets
    int t;
    int **fast_sets;

    // Changed list
    int n;
    int *changed;

    long long offset;

    // Reduction stack
    int m;
    void **reduction;
} red_data;

red_data *red_data_init(graph *g);

void red_data_free(red_data *rd);

typedef int (*func_reduce)(graph *, int, red_data *, void **);
typedef void (*func_restore)(graph *, red_data *, void *);
typedef void (*func_lift)(int *, void *);
typedef void (*func_clear)(void *);

typedef struct
{
    func_reduce reduce;
    func_restore restore;
    func_lift lift;
    func_clear clear;
} reduction;

void reduce_graph(graph *g, int n_red, ...);

void lift_solution(graph *g, restore *rd);

// Low Degree Reduction Rules

// Degree Zero

int degree_zero_reduction(graph *g, int u, red_data *rd, void **rec);

void degree_zero_reconstruct_graph(graph *g, red_data *rd, void *rec);

void degree_zero_reconstruct_solution(int *I, void *rec);

void degree_zero_free(void *rec);

const reduction degree_zero = {
    .reduce = degree_zero_reduction,
    .restore = degree_zero_reconstruct_graph,
    .lift = degree_zero_reconstruct_solution,
    .clear = degree_zero_free};

// Degree One

int degree_one_reduction(graph *g, int u, red_data *rd, void **rec);

void degree_one_reconstruct_graph(graph *g, red_data *rd, void *rec);

void degree_one_reconstruct_solution(int *I, void *rec);

void degree_one_free(void *rec);

const reduction degree_zero = {
    .reduce = degree_one_reduction,
    .restore = degree_one_reconstruct_graph,
    .lift = degree_one_reconstruct_solution,
    .clear = degree_one_free};