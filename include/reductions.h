#pragma once

#include "hypergraph.h"
#include "graph.h"

int reduction_vertex_domination(hypergraph *g);

int reduction_edge_domination(hypergraph *g);

graph *reduction_hitting_set_to_mwis(hypergraph *g);

// MWIS reductions

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
    int n_changed;
    int *changed;
} reduction_data;

typedef struct
{
    // Mandatory offset when reduction is successful
    long long offset;

    // Optional variables for reconstruction
    int n, m, u, v, w, x, y, z;
    // Use this to allocate additional data if necessary
    void *data;
} reconstruction_data;

typedef int (*func_reduce)(graph *, int, reduction_data *, reconstruction_data *);
typedef void (*func_restore)(graph *, reconstruction_data *);
typedef void (*func_lift_solution)(int *, reconstruction_data *);
typedef void (*func_clean)(reconstruction_data *);

typedef struct
{
    func_reduce reduce;
    func_restore restore;
    func_lift_solution lift_solution;
    func_clean clean;
    int global;
} reduction;

reduction_data *reduction_data_init(graph *g);

void reduction_data_increase(reduction_data *rd);

void reduction_data_free(reduction_data *rd);

// Low Degree Reduction Rules

// Degree Zero

int degree_zero_reduce(graph *g, int u, reduction_data *rd, reconstruction_data *rc);

void degree_zero_reconstruct_graph(graph *g, reconstruction_data *rc);

void degree_zero_reconstruct_solution(int *I, reconstruction_data *rc);

void degree_zero_free(reconstruction_data *rc);

static const reduction degree_zero_reduction = {
    .reduce = degree_zero_reduce,
    .restore = degree_zero_reconstruct_graph,
    .lift_solution = degree_zero_reconstruct_solution,
    .clean = degree_zero_free,
    .global = 0};

// Degree One

int degree_one_reduce(graph *g, int u, reduction_data *rd, reconstruction_data *rc);

void degree_one_reconstruct_graph(graph *g, reconstruction_data *rc);

void degree_one_reconstruct_solution(int *I, reconstruction_data *rc);

void degree_one_free(reconstruction_data *rc);

static const reduction degree_one_reduction = {
    .reduce = degree_one_reduce,
    .restore = degree_one_reconstruct_graph,
    .lift_solution = degree_one_reconstruct_solution,
    .clean = degree_one_free,
    .global = 0};

int domination_reduce(graph *g, int u, reduction_data *rd, reconstruction_data *rc);

void domination_reconstruct_graph(graph *g, reconstruction_data *rc);

void domination_reconstruct_solution(int *I, reconstruction_data *rc);

void domination_free(reconstruction_data *rc);

static const reduction domination_reduction = {
    .reduce = domination_reduce,
    .restore = domination_reconstruct_graph,
    .lift_solution = domination_reconstruct_solution,
    .clean = domination_free,
    .global = 0};