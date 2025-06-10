#include "local_search_hs.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#define MAX_GUESS 128
#define DEFAULT_QUEUE_SIZE 32

double ls_hs_get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

local_search_hs *local_search_hs_init(graph_csr *g, unsigned int seed)
{
    local_search_hs *ls = malloc(sizeof(local_search_hs));

    ls->hitting_set = malloc(sizeof(int) * g->n);

    ls->queue = malloc(sizeof(int) * g->n);
    ls->in_queue = malloc(sizeof(int) * g->n);
    ls->prev_queue = malloc(sizeof(int) * g->n);
    ls->in_prev_queue = malloc(sizeof(int) * g->n);

    ls->max_queue = DEFAULT_QUEUE_SIZE;
    ls->score = malloc(sizeof(int) * g->n);
    ls->cover_count = malloc(sizeof(int) * g->m);
    ls->one_tight = malloc(sizeof(int) * g->m);
    ls->tabu = malloc(sizeof(int) * g->n);
    ls->T = malloc(sizeof(int) * g->n);

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

    free(ls->queue);
    free(ls->in_queue);
    free(ls->prev_queue);
    free(ls->in_prev_queue);

    free(ls->score);
    free(ls->cover_count);
    free(ls->one_tight);
    free(ls->tabu);
    free(ls->T);

    free(ls->log);
}

void local_search_hs_reset(graph_csr *g, local_search_hs *ls)
{
    ls->cost = g->n;
    ls->balance = 0;
    ls->time = 0.0;
    ls->time_ref = ls_hs_get_wtime();

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
        ls->prev_queue[u] = 0;
        ls->in_prev_queue[u] = 0;
    }
    for (int e = 0; e < g->m; e++)
    {
        ls->cover_count[e] = g->V[g->n + e + 1] - g->V[g->n + e];
        ls->one_tight[e] = g->E[g->V[g->n + e]];

        if (ls->cover_count[e] == 1)
            ls->score[ls->one_tight[e]]++;

        if (ls->cover_count[e] > 1)
            ls->balance++;
    }
}

static inline void local_search_hs_shuffle(int *list, int n, unsigned int *seed)
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
    assert(!ls->hitting_set[u]);

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
    ls->T[u]++;

    // if (!ls->in_queue[u])
    // {
    //     ls->in_queue[u] = 1;
    //     ls->queue[ls->queue_count++] = u;
    // }

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        ls->cover_count[e]++;

        if (ls->cover_count[e] > 2)
            continue;

        ls->balance++;

        // for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
        // {
        //     int v = g->E[j];

        //     if (!ls->in_queue[v])
        //     {
        //         ls->in_queue[v] = 1;
        //         ls->queue[ls->queue_count++] = v;
        //     }
        // }

        int v = ls->one_tight[e];
        ls->score[v]--;

        if (ls->score[v] == 0 && !ls->in_queue[v] && !ls->tabu[v])
        {
            ls->in_queue[v] = 1;
            ls->queue[ls->queue_count++] = v;
        }
    }
}

void local_search_hs_remove_vertex(graph_csr *g, local_search_hs *ls, int u)
{
    assert(ls->hitting_set[u]);

    if (ls->score[u] > 0)
    {
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            if (ls->cover_count[e] > 1)
                continue;

            int d = g->V[g->n + e + 1] - g->V[g->n + e];
            if (d > 2)
                continue;

            int v = g->E[g->V[g->n + e]];
            if (v == u)
                v = g->E[g->V[g->n + e] + 1];

            if (ls->tabu[v])
                return;

            local_search_hs_add_vertex(g, ls, v);
        }
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            if (ls->cover_count[e] > 1)
                continue;

            int d = g->V[g->n + e + 1] - g->V[g->n + e];
            int v = g->E[g->V[g->n + e] + (rand_r(&ls->seed) % d)];
            while (v == u)
                v = g->E[g->V[g->n + e] + (rand_r(&ls->seed) % d)];

            if (ls->tabu[v])
                return;

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
    ls->T[u]++;

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        ls->cover_count[e]--;

        if (ls->cover_count[e] > 1)
            continue;

        assert(ls->cover_count[e] == 1);

        ls->balance--;

        for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
        {
            int v = g->E[j];

            // if (!ls->in_queue[v])
            // {
            //     ls->in_queue[v] = 1;
            //     ls->queue[ls->queue_count++] = v;
            // }

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
    assert(ls->hitting_set[u]);

    if (ls->tabu[u])
        return;

    if (ls->score[u] > 0)
    {
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            if (ls->cover_count[e] > 1)
                continue;

            int options = 0, opt = 0;
            for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
            {
                int v = g->E[j];
                if (v == u || ls->tabu[v])
                    continue;

                options++;
                opt = v;
            }

            if (options == 0)
                return;
        }
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            if (ls->cover_count[e] > 1)
                continue;

            int options = 0, opt = 0;
            for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
            {
                int v = g->E[j];
                if (v == u || ls->tabu[v])
                    continue;

                options++;
                opt = v;
            }

            if (options == 1)
            {
                local_search_hs_add_vertex(g, ls, opt);
                ls->tabu[opt] = 1;
            }
            else
            {
                local_search_hs_add_vertex(g, ls, opt);
            }
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
    ls->tabu[u] = 1;

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        ls->cover_count[e]--;

        if (ls->cover_count[e] > 1)
            continue;

        ls->balance--;

        for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
        {
            int v = g->E[j];

            // if (!ls->in_queue[v])
            // {
            //     ls->in_queue[v] = 1;
            //     ls->queue[ls->queue_count++] = v;
            // }

            if (!ls->hitting_set[v])
                continue;

            ls->one_tight[e] = v;
            ls->score[v]++;

            break;
        }
    }
}

int local_search_hs_one_two_swap(graph_csr *g, local_search_hs *ls, int u)
{
    assert(!ls->hitting_set[u]);

    int n = 0;
    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        ls->cover_count[e]++;
        if (ls->cover_count[e] > 2)
            continue;

        int v = ls->one_tight[e];
        ls->score[v]--;
        if (ls->score[v] == 0)
            ls->T[n++] = v;
    }

    int found = 0, w1, w2;
    if (n >= 2)
    {
        // Found all remove candidates
        local_search_hs_shuffle(ls->T, n, &ls->seed);
        if (n > 128)
            n = 128;

        // Try every pair of vertiecs
        for (int i = 0; i < n && !found; i++)
        {
            int v1 = ls->T[i];
            for (int j = i + 1; j < n && !found; j++)
            {
                int v2 = ls->T[j];

                int i1 = g->V[v1], i2 = g->V[v2];
                int valid_pair = 1;
                while (i1 < g->V[v1 + 1] && i2 < g->V[v2 + 1] && valid_pair)
                {
                    if (g->E[i1] < g->E[i2])
                        i1++;
                    else if (g->E[i1] > g->E[i2])
                        i2++;
                    else
                    {
                        if (ls->cover_count[g->E[i1]] == 2)
                            valid_pair = 0;

                        i1++;
                        i2++;
                    }
                }

                if (valid_pair)
                {
                    found = 1;
                    w1 = v1;
                    w2 = v2;
                }
            }
        }
    }

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        ls->cover_count[e]--;
        if (ls->cover_count[e] > 1)
            continue;

        int v = ls->one_tight[e];
        ls->score[v]++;
    }

    if (found)
    {
        local_search_hs_add_vertex(g, ls, u);
        local_search_hs_remove_vertex(g, ls, w1);
        local_search_hs_remove_vertex(g, ls, w2);
        return 1;
    }
    return 0;
}

// void local_search_aad_move(graph_csr *g, int u, local_search_hs *ls)
// {
//     if (!ls->hitting_set[u] || ls->score[u] != 1)
//         return;

//     int *T = ls->T;
//     for (int i = 0; i < g->n; i++)
//         T[i] = 0;

//     T[u] = 1;
//     local_search_hs_remove_vertex(g, ls, u);

//     int v = ls->log[ls->log_count - 1];
//     T[v] = 1;

// }

void ls_hs_swap(int **a, int **b)
{
    int *t = *a;
    *a = *b;
    *b = t;
}

void local_search_hs_greedy(graph_csr *g, local_search_hs *ls)
{
    local_search_hs_shuffle(ls->queue, ls->queue_count, &ls->seed);

    int n = ls->queue_count;
    ls->queue_count = 0;
    while (n > 0)
    {
        ls_hs_swap(&ls->queue, &ls->prev_queue);
        ls_hs_swap(&ls->in_queue, &ls->in_prev_queue);

        for (int i = 0; i < n; i++)
        {
            int u = ls->prev_queue[i];
            ls->in_prev_queue[u] = 0;

            if (!ls->tabu[u] && ls->hitting_set[u] && ls->score[u] == 0)
                local_search_hs_remove_vertex(g, ls, u);
            // else if (!ls->hitting_set[u])
            //     local_search_hs_one_two_swap(g, ls, u);
        }

        local_search_hs_shuffle(ls->queue, ls->queue_count, &ls->seed);

        n = ls->queue_count;
        ls->queue_count = 0;
    }
}

void local_search_hs_perturbe(graph_csr *g, local_search_hs *ls)
{
    int it = 0;
    int to_add = 1; // + (rand_r(&ls->seed) % 4);
    while (it++ < to_add && ls->queue_count < ls->max_queue)
    {
        int u = rand_r(&ls->seed) % g->n;
        while (ls->tabu[u])
            u = rand_r(&ls->seed) % g->n;
        // if (ls->queue_count == 0)
        //     u = rand_r(&ls->seed) % g->n;
        // else
        //     u = ls->queue[rand_r(&ls->seed) % ls->queue_count];
        // int _t = 0;
        // while ((ls->score[u] > 0) && _t++ < MAX_GUESS)
        //     u = rand_r(&ls->seed) % g->n;

        if (ls->hitting_set[u])
            local_search_hs_remove_vertex(g, ls, u);
        else
            local_search_hs_add_vertex(g, ls, u);
    }
}

void local_search_hs_explore(graph_csr *g, local_search_hs *ls, double tl, volatile sig_atomic_t *tle,
                             long long il, int offset, int verbose)
{
    int best = ls->cost;
    long long c = 0;

    double start = ls_hs_get_wtime();

    int best_outer = ls->cost;
    int *best_hs = malloc(sizeof(int) * g->n);

    best_outer = ls->cost;
    for (int i = 0; i < g->n; i++)
        best_hs[i] = ls->hitting_set[i];

    // ls->age = 0;
    // for (int i = 0; i < g->n; i++)
    //     ls->T[i] = ls->hitting_set[i];

    if (verbose)
    {
        if (il < LLONG_MAX)
            printf("Running baseline local search for %.2lf seconds or %lld iterations\n", tl, il);
        else
            printf("Running baseline local search for %.2lf seconds\n", tl);
        printf("%11s %12s %8s %8s\n", "It.", "HS", "Ts", "Te");
        printf("\r%10lld: %12d %8.2lf %8.2lf", c, offset + ls->cost, ls->time, ls_hs_get_wtime() - start);
        fflush(stdout);
    }

    ls->log_enabled = 0;
    local_search_hs_greedy(g, ls);
    c++;

    int second_counter = 1;

    if (ls->cost < best)
    {
        best = ls->cost;
        ls->time = ls_hs_get_wtime() - ls->time_ref;
    }

    while (c < il)
    {
        c++;
        ls->age++;
        if ((c & ((1 << 7) - 1)) == 0)
        {
            // c = 0;
            double elapsed = ls_hs_get_wtime() - start;
            if (elapsed > tl || *tle)
                break;

            // int imp = 1;
            // while (imp)
            // {
            //     imp = 0;
            //     for (int i = 0; i < g->n; i++)
            //     {
            //         if (ls->hitting_set[i])
            //             continue;

            //         imp |= local_search_hs_one_two_swap(g, ls, i);
            //     }
            // }
            // if (ls->cost < best)
            // {
            //     best = ls->cost;
            //     ls->time = ls_hs_get_wtime() - ls->time_ref;
            // }

            if (verbose)
            {
                printf("\r%10lld: %12d %8.2lf %8.2lf %10d %10d", c, offset + ls->cost, ls->time, elapsed, best_outer + offset, ls->balance);
                fflush(stdout);
            }

            if ((c & ((1 << 17) - 1)) == 0) // (c & ((1 << 17) - 1)) == 0
            {
                // for (int i = 0; i < g->n; i++)
                //     ls->tabu[i] = 0;

                c = 0;
                ls->log_enabled = 0;
                if (ls->cost < best_outer)
                {
                    best_outer = ls->cost;
                    for (int i = 0; i < g->n; i++)
                        best_hs[i] = ls->hitting_set[i];

                    // int val_cost = 0;
                    // for (int i = 0; i < g->n; i++)
                    // {
                    //     if (best_hs[i])
                    //         val_cost++;
                    // }
                    // for (int e = 0; e < g->m; e++)
                    // {
                    //     int hit = 0;
                    //     for (int i = g->V[g->n + e]; i < g->V[g->n + e + 1]; i++)
                    //     {
                    //         if (best_hs[g->E[i]])
                    //             hit = 1;
                    //     }
                    //     if (!hit)
                    //     {
                    //         printf("\nInvalid solution\n");
                    //         exit(0);
                    //     }
                    // }
                }
                else if (ls->cost > best_outer)
                {
                    for (int i = 0; i < g->n; i++)
                        if (!ls->hitting_set[i])
                            local_search_hs_add_vertex(g, ls, i);
                    for (int i = 0; i < g->n; i++)
                        if (!best_hs[i])
                            local_search_hs_remove_vertex(g, ls, i);
                }

                int n_rand = 1 + (rand_r(&ls->seed) % 32);
                for (int i = 0; i < n_rand; i++)
                {
                    int u = rand_r(&ls->seed) % g->n;
                    while (ls->tabu[u])
                        u = rand_r(&ls->seed) % g->n;

                    if (!ls->hitting_set[u])
                    {
                        local_search_hs_add_vertex(g, ls, u);
                        // ls->tabu[u] = 1;
                    }
                    else
                    {
                        local_search_hs_remove_vertex(g, ls, u);
                        // local_search_hs_exclude_vertex(g, ls, u);
                    }
                }
                // int u = rand_r(&ls->seed) % g->n;
                // while (ls->hitting_set[u])
                //     u = rand_r(&ls->seed) % g->n;

                // local_search_hs_add_vertex(g, ls, u);
                // ls->tabu[u] = 1;

                best = ls->cost;
            }
        }

        int balance = ls->balance;

        ls->log_count = 0;
        ls->log_enabled = 1;

        local_search_hs_perturbe(g, ls);
        local_search_hs_greedy(g, ls);

        if (ls->cost < best)
        {
            best = ls->cost;
            ls->time = ls_hs_get_wtime() - ls->time_ref;
        }
        else if (ls->cost > best) //  || (ls->cost == best && ls->balance < balance)
        {
            local_search_hs_unwind(g, ls, 0);
        }
    }
    if (verbose)
        printf("\n");

    // for (int i = 0; i < g->n; i++)
    //     ls->tabu[i] = 0;
    if (ls->cost > best_outer)
    {
        ls->cost = best_outer;
        for (int i = 0; i < g->n; i++)
        {
            ls->hitting_set[i] = best_hs[i];
        }
        // for (int i = 0; i < g->n; i++)
        //     if (!ls->hitting_set[i])
        //         local_search_hs_add_vertex(g, ls, i);
        // for (int i = 0; i < g->n; i++)
        //     if (!best_hs[i])
        //         local_search_hs_remove_vertex(g, ls, i);
    }

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