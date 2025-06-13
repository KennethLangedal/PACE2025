#include "local_search_hs.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

double ls_hs_get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

local_search_hs *local_search_hs_init(graph_csr *g, unsigned int seed)
{
    local_search_hs *ls = malloc(sizeof(local_search_hs));

    ls->time = 0.0;
    ls->time_ref = ls_hs_get_wtime();

    ls->hitting_set = malloc(sizeof(int) * g->n);
    ls->best_hitting_set = malloc(sizeof(int) * g->n);

    ls->cost = g->n;
    ls->best_cost = g->n;
    for (int i = 0; i < g->n; i++)
        ls->best_hitting_set[i] = 1;

    ls->queue = malloc(sizeof(int) * g->n);
    ls->in_queue = malloc(sizeof(int) * g->n);

    ls->score = malloc(sizeof(int) * g->n);
    ls->cover_count = malloc(sizeof(int) * g->m);
    ls->one_tight = malloc(sizeof(int) * g->m);
    ls->tabu = malloc(sizeof(int) * g->n);

    ls->log_alloc = g->n;
    ls->log = malloc(sizeof(int) * ls->log_alloc);
    ls->log_enabled = 0;

    ls->seed = seed;

    local_search_hs_reset(g, ls);

    return ls;
}

void local_search_hs_free(local_search_hs *ls)
{
    free(ls->hitting_set);
    free(ls->best_hitting_set);

    free(ls->queue);
    free(ls->in_queue);

    free(ls->score);
    free(ls->cover_count);
    free(ls->one_tight);

    free(ls->log);
}

void local_search_hs_reset(graph_csr *g, local_search_hs *ls)
{
    if (ls->cost < ls->best_cost)
    {
        ls->best_cost = ls->cost;
        for (int i = 0; i < g->n; i++)
            ls->best_hitting_set[i] = ls->hitting_set[i];
    }

    ls->cost = g->n;

    ls->queue_count = g->n;

    ls->log_count = 0;
    ls->log_enabled = 0;

    for (int u = 0; u < g->n; u++)
    {
        ls->hitting_set[u] = 1;

        ls->score[u] = 0;
        ls->tabu[u] = 0;

        ls->queue[u] = u;
        ls->in_queue[u] = 1;
    }
    for (int e = 0; e < g->m; e++)
    {
        ls->cover_count[e] = g->V[g->n + e + 1] - g->V[g->n + e];
        ls->one_tight[e] = -1;

        assert(ls->cover_count[e] > 1);
    }
}

void local_search_hs_shuffle(int *list, int n, unsigned int *seed)
{
    for (int i = 0; i < n - 1; i++)
    {
        int j = i + (rand_r(seed) % (n - i));
        int t = list[j];
        list[j] = list[i];
        list[i] = t;
    }
}

void local_search_hs_add_vertex(graph_csr *g, local_search_hs *ls, int u)
{
    if (ls->hitting_set[u] || ls->tabu[u])
        return;

    if (ls->log_enabled)
    {
        if (ls->log_count >= ls->log_alloc)
        {
            ls->log_alloc *= 2;
            ls->log = realloc(ls->log, sizeof(int) * ls->log_alloc);
        }
        ls->log[ls->log_count++] = u;
    }

    ls->hitting_set[u] = 1;
    ls->cost++;

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        ls->cover_count[e]++;

        if (ls->cover_count[e] > 2)
            continue;

        int v = ls->one_tight[e];
        ls->score[v]--;

        assert(v != u);
        assert(ls->hitting_set[v]);

        if (ls->score[v] == 0 && !ls->in_queue[v])
        {
            ls->in_queue[v] = 1;
            ls->queue[ls->queue_count++] = v;
        }
    }
}

void local_search_hs_remove_vertex(graph_csr *g, local_search_hs *ls, int u)
{
    if (!ls->hitting_set[u] || ls->tabu[u])
        return;

    if (ls->score[u] > 0)
    {
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            int d = g->V[g->n + e + 1] - g->V[g->n + e];
            if (ls->cover_count[e] > 1 || d > 2)
                continue;

            int v = g->E[g->V[g->n + e]];
            if (v == u)
                v = g->E[g->V[g->n + e] + 1];

            local_search_hs_add_vertex(g, ls, v);
        }
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            int d = g->V[g->n + e + 1] - g->V[g->n + e];
            if (ls->cover_count[e] > 1)
                continue;

            int any = 0;
            for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
            {
                int v = g->E[j];
                if (v != u && !ls->tabu[v])
                    any = 1;
            }

            assert(any);

            int v = g->E[g->V[g->n + e] + (rand_r(&ls->seed) % d)];
            while (v == u || ls->tabu[v])
                v = g->E[g->V[g->n + e] + (rand_r(&ls->seed) % d)];

            local_search_hs_add_vertex(g, ls, v);
        }
    }

    assert(ls->score[u] == 0);

    if (ls->log_enabled)
    {
        if (ls->log_count >= ls->log_alloc)
        {
            ls->log_alloc *= 2;
            ls->log = realloc(ls->log, sizeof(int) * ls->log_alloc);
        }
        ls->log[ls->log_count++] = u;
    }

    ls->hitting_set[u] = 0;
    ls->cost--;

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        ls->cover_count[e]--;

        if (ls->cover_count[e] > 1)
            continue;

        assert(ls->cover_count[e] == 1);

        for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
        {
            int v = g->E[j];

            if (!ls->hitting_set[v])
                continue;

            ls->one_tight[e] = v;
            ls->score[v]++;

            break;
        }
    }
}

void local_search_hs_exclude_vertex(graph_csr *g, local_search_hs *ls, int u)
{
    if (!ls->hitting_set[u] || ls->tabu[u])
        return;

    int valid = 1;
    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        int any = 0;
        for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
        {
            int v = g->E[j];
            if (v != u && (!ls->tabu[v] || (ls->tabu[v] && ls->hitting_set[v])))
            {
                any = 1;
                break;
            }
        }
        if (!any)
        {
            valid = 0;
            break;
        }
    }
    if (!valid)
        return;

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        int n = 0, ok = 0, opt = -1;
        for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
        {
            int v = g->E[j];
            if (v == u)
                continue;

            if (ls->tabu[v] && ls->hitting_set[v])
                ok = 1;
            else if (!ls->tabu[v])
            {
                n++;
                opt = v;
            }
        }

        if (ok)
            continue;

        assert(n > 0);

        if (n == 1)
        {
            local_search_hs_add_vertex(g, ls, opt);
            assert(!ls->tabu[opt] && ls->hitting_set[opt]);
            ls->tabu[opt] = 1;
        }
    }

    local_search_hs_remove_vertex(g, ls, u);
    assert(!ls->hitting_set[u] && !ls->tabu[u]);
    ls->tabu[u] = 1;
}

void local_search_hs_greedy(graph_csr *g, local_search_hs *ls)
{
    local_search_hs_shuffle(ls->queue, ls->queue_count, &ls->seed);

    for (int i = 0; i < ls->queue_count; i++)
    {
        int u = ls->queue[i];
        ls->in_queue[u] = 0;

        if (ls->hitting_set[u] && ls->score[u] == 0)
            local_search_hs_remove_vertex(g, ls, u);
    }

    ls->queue_count = 0;
}

void local_search_hs_perturbe(graph_csr *g, local_search_hs *ls)
{
    int u = rand_r(&ls->seed) % g->n;

    if (ls->hitting_set[u])
        local_search_hs_remove_vertex(g, ls, u);
    else
        local_search_hs_add_vertex(g, ls, u);
}

void local_search_hs_explore(graph_csr *g, local_search_hs *ls, double tl, volatile sig_atomic_t *tle, int offset, int verbose)
{
    long long c = 0;

    double start = ls_hs_get_wtime();

    ls->log_enabled = 0;
    local_search_hs_greedy(g, ls);
    c++;

    ls->time = ls_hs_get_wtime() - ls->time_ref;

    if (ls->cost < ls->best_cost)
    {
        ls->best_cost = ls->cost;
        for (int i = 0; i < g->n; i++)
            ls->best_hitting_set[i] = ls->hitting_set[i];
    }

    if (verbose)
    {
        printf("Running baseline local search for %.2lf seconds\n", tl);
        printf("%11s %12s %8s %8s %8s\n", "It.", "HS", "Ts", "Te", "Best");
        printf("\r%10lld: %12d %8.2lf %8.2lf %12d", c, offset + ls->cost, ls->time, ls_hs_get_wtime() - start, ls->best_cost + offset);
        fflush(stdout);
    }

    int best = ls->cost;
    int best_outer = ls->cost;
    int *best_hs = malloc(sizeof(int) * g->n);
    for (int i = 0; i < g->n; i++)
        best_hs[i] = ls->hitting_set[i];

    while (1)
    {
        c++;
        if ((c & ((1 << 7) - 1)) == 0)
        {
            if (c > (50 * g->n))
            {
                c = 0;
                ls->log_enabled = 0;

                if (ls->cost < ls->best_cost)
                {
                    ls->best_cost = ls->cost;
                    for (int i = 0; i < g->n; i++)
                        ls->best_hitting_set[i] = ls->hitting_set[i];
                }
                if (ls->cost < best_outer)
                {
                    best_outer = ls->cost;
                    for (int i = 0; i < g->n; i++)
                        best_hs[i] = ls->hitting_set[i];
                }
                else if (ls->cost > best_outer)
                {
                    for (int i = 0; i < g->n; i++)
                        local_search_hs_add_vertex(g, ls, i);
                    for (int i = 0; i < g->n; i++)
                        if (!best_hs[i])
                            local_search_hs_remove_vertex(g, ls, i);
                }

                assert(ls->cost == best_outer);

                int n_rand = 1 + (rand_r(&ls->seed) % 32);

                for (int i = 0; i < n_rand; i++)
                {
                    int u = rand_r(&ls->seed) % g->n;

                    if (!ls->hitting_set[u])
                        local_search_hs_add_vertex(g, ls, u);
                    else
                        local_search_hs_remove_vertex(g, ls, u);
                }
                best = ls->cost;
            }

            double elapsed = ls_hs_get_wtime() - start;
            if (elapsed > tl || *tle)
                break;

            if (verbose)
            {
                printf("\r%10lld: %12d %8.2lf %8.2lf %12d", c, offset + ls->cost, ls->time, elapsed, ls->best_cost + offset);
                fflush(stdout);
            }
        }

        ls->log_count = 0;
        ls->log_enabled = 1;

        local_search_hs_perturbe(g, ls);
        local_search_hs_greedy(g, ls);

        if (ls->cost < best)
        {
            best = ls->cost;
            ls->time = ls_hs_get_wtime() - ls->time_ref;
        }
        else if (ls->cost > best)
        {
            local_search_hs_unwind(g, ls, 0);
        }
    }

    if (verbose)
        printf("\n");

    free(best_hs);
}

void local_search_hs_unwind(graph_csr *g, local_search_hs *ls, int log_t)
{
    ls->log_enabled = 0;
    while (ls->log_count > log_t)
    {
        ls->log_count--;
        int u = ls->log[ls->log_count];

        if (ls->hitting_set[u])
            local_search_hs_remove_vertex(g, ls, u);
        else
            local_search_hs_add_vertex(g, ls, u);
    }
}