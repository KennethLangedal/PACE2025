#include "local_search.h"

#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define MAX_GUESSES 512
#define LOG_LIMIT 8
#define DEFAULT_MAX_QUEUE (1 << 15)

static inline double local_search_get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

static inline void local_search_shuffle(int *list, int n, unsigned int *seed)
{
    for (int i = 0; i < n - 1; i++)
    {
        int j = i + (rand_r(seed) % (n - i));
        int t = list[j];
        list[j] = list[i];
        list[i] = t;
    }
}

static inline void local_search_queue_vertex(local_search *ls, int u)
{
    if (!ls->in_queue[u])
    {
        ls->in_queue[u] = 1;
        ls->queue[ls->queue_count] = u;
        ls->queue_count++;
    }
}

static inline void local_search_print_update(local_search *ls)
{
    printf("\r%d %.6lf    ", ls->cost, ls->time);
    fflush(stdout);
}

static inline void local_search_swap(int **a, int **b)
{
    int *t = *a;
    *a = *b;
    *b = t;
}

local_search *local_search_init(graph *g, unsigned int seed)
{
    local_search *ls = malloc(sizeof(local_search));

    ls->cost = g->n;
    ls->dominating_set = malloc(sizeof(int) * g->n);

    ls->time = 0.0;
    ls->time_ref = local_search_get_wtime();

    ls->max_queue = DEFAULT_MAX_QUEUE;
    ls->queue_count = g->n;
    ls->queue = malloc(sizeof(int) * g->n);
    ls->in_queue = malloc(sizeof(int) * g->n);
    ls->prev_queue = malloc(sizeof(int) * g->n);
    ls->in_prev_queue = malloc(sizeof(int) * g->n);

    ls->tightness = malloc(sizeof(int) * g->n);

    ls->log_count = 0;
    ls->log_enabled = 0;
    ls->log = malloc(sizeof(int) * g->n * LOG_LIMIT);

    ls->seed = seed;

    for (int u = 0; u < g->n; u++)
    {
        ls->dominating_set[u] = 1;

        ls->queue[u] = u;
        ls->in_queue[u] = 1;
        ls->prev_queue[u] = 0;
        ls->in_prev_queue[u] = 0;

        ls->tightness[u] = (g->V[u + 1] - g->V[u]) + 1;
    }

    return ls;
}

void local_search_free(local_search *ls)
{
    free(ls->dominating_set);

    free(ls->queue);
    free(ls->in_queue);
    free(ls->prev_queue);
    free(ls->in_prev_queue);

    free(ls->tightness);

    free(ls->log);

    free(ls);
}

// Run local search for tl seconds

void local_search_explore(graph *g, local_search *ls, double tl, int verbose)
{
    int c = 0, q = 0;

    int best = ls->cost;

    if (verbose)
    {
        printf("Running local search for %.2lf seconds\n", tl);
        local_search_print_update(ls);
    }

    double start = local_search_get_wtime();

    local_search_greedy(g, ls);

    if (ls->cost < best)
    {
        best = ls->cost;
        ls->time = local_search_get_wtime() - ls->time_ref;
        if (verbose)
            local_search_print_update(ls);
    }

    while (1)
    {
        if ((c++ & ((1 << 12) - 1)) == 0)
        {
            c = 0;
            if (local_search_get_wtime() - start > tl)
                break;
        }

        ls->log_count = 0;
        ls->log_enabled = 1;

        local_search_perturbe(g, ls);

        local_search_greedy(g, ls);

        if (ls->cost < best)
        {
            best = ls->cost;
            ls->time = local_search_get_wtime() - ls->time_ref;
            ls->log_count = 0;
            if (verbose)
                local_search_print_update(ls);
        }
        if (ls->cost > best)
        {
            local_search_unwind(g, ls, 0);
        }
    }
    if (verbose)
        printf("\n");
}

// Helper functions

void local_search_add_vertex(graph *g, local_search *ls, int u)
{
    assert(!ls->dominating_set[u]);

    ls->dominating_set[u] = 1;
    ls->tightness[u]++;
    ls->cost++;

    if (ls->log_enabled)
        ls->log[ls->log_count++] = u + 1;

    local_search_queue_vertex(ls, u);

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int v = g->E[i];
        ls->tightness[v]++;

        local_search_queue_vertex(ls, u);

        for (int j = g->V[v]; j < g->V[v + 1]; j++)
            local_search_queue_vertex(ls, g->E[j]);
    }
}

void local_search_remove_vertex(graph *g, local_search *ls, int u)
{
    assert(ls->dominating_set[u]);

    if (ls->log_enabled)
        ls->log[ls->log_count++] = (u * -1) - 1;

    ls->dominating_set[u] = 0;
    ls->tightness[u]--;
    ls->cost--;

    // local_search_queue_vertex(ls, u);

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int v = g->E[i];
        ls->tightness[v]--;

        // local_search_queue_vertex(ls, v);
    }
}

void local_search_greedy(graph *g, local_search *ls)
{
    local_search_shuffle(ls->queue, ls->queue_count, &ls->seed);

    int n = ls->queue_count;
    ls->queue_count = 0;
    while (n > 0)
    {
        local_search_swap(&ls->queue, &ls->prev_queue);
        local_search_swap(&ls->in_queue, &ls->in_prev_queue);

        for (int i = 0; i < n; i++)
        {
            int u = ls->prev_queue[i];
            ls->in_prev_queue[u] = 0;

            if (!ls->dominating_set[u] || ls->tightness[u] == 1)
                continue;

            int f = 0;
            for (int j = g->V[u]; j < g->V[u + 1]; j++)
                f |= ls->tightness[g->E[j]] == 1;

            if (!f)
                local_search_remove_vertex(g, ls, u);
        }

        local_search_shuffle(ls->queue, ls->queue_count, &ls->seed);

        n = ls->queue_count;
        ls->queue_count = 0;
    }
}

void local_search_perturbe(graph *g, local_search *ls)
{
    int u = rand_r(&ls->seed) % g->n;
    int q = 0;
    while (q++ < MAX_GUESSES && ls->dominating_set[u])
        u = rand_r(&ls->seed) % g->n;

    if (ls->dominating_set[u])
        return;

    local_search_add_vertex(g, ls, u);

    for (int i = 0; i < MAX_GUESSES &&
                    ls->queue_count > 0 &&
                    ls->queue_count < ls->max_queue;
         i++)
    {
        int v = ls->queue[rand_r(&ls->seed) % ls->queue_count];
        int q = 0;
        while (q++ < MAX_GUESSES && ls->dominating_set[v])
            v = ls->queue[rand_r(&ls->seed) % ls->queue_count];

        if (ls->dominating_set[v])
            continue;

        local_search_add_vertex(g, ls, v);
    }
}

void local_search_unwind(graph *g, local_search *ls, int t)
{
    ls->log_enabled = 0;
    while (ls->log_count > t)
    {
        ls->log_count--;
        int u = ls->log[ls->log_count];

        if (u < 0)
            local_search_add_vertex(g, ls, (u * -1) - 1);
        else
            local_search_remove_vertex(g, ls, u - 1);
    }
}

int local_search_validate_solution(graph *g, local_search *ls)
{
    int cost = 0, val = 1;
    for (int u = 0; u < g->n; u++)
    {
        int any = ls->dominating_set[u];
        cost += any;
        for (int i = g->V[u]; i < g->V[u + 1] && !any; i++)
            any |= ls->dominating_set[g->E[i]];
        val &= any;
    }

    return val && cost == ls->cost;
}