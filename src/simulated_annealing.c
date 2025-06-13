#include "simulated_annealing.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

double sa_get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

simulated_annealing *simulated_annealing_init(graph_csr *g, unsigned int seed)
{
    simulated_annealing *sa = malloc(sizeof(simulated_annealing));

    sa->hitting_set = malloc(sizeof(int) * g->n);
    sa->best_hitting_set = malloc(sizeof(int) * g->n);

    sa->best_cost = g->n;
    for (int i = 0; i < g->n; i++)
        sa->best_hitting_set[i] = 1;

    sa->score = malloc(sizeof(int) * g->n);
    sa->cover_count = malloc(sizeof(int) * g->m);
    sa->one_tight = malloc(sizeof(int) * g->m);

    sa->k = 999999999ll;

    sa->seed = seed;

    sa->time = 0.0;
    sa->time_ref = sa_get_wtime();

    simulated_annealing_reset(g, sa);

    return sa;
}

void simulated_annealing_free(simulated_annealing *sa)
{
    free(sa->hitting_set);
    free(sa->best_hitting_set);

    free(sa->score);
    free(sa->cover_count);
    free(sa->one_tight);

    free(sa);
}

void simulated_annealing_reset(graph_csr *g, simulated_annealing *sa)
{
    sa->cost = g->n;

    for (int u = 0; u < g->n; u++)
    {
        sa->hitting_set[u] = 1;
        sa->score[u] = 0;
    }
    for (int e = 0; e < g->m; e++)
    {
        sa->cover_count[e] = g->V[g->n + e + 1] - g->V[g->n + e];
        sa->one_tight[e] = -1;

        assert(sa->cover_count[e] > 1);
    }
}

void simulated_annealing_add_vertex(graph_csr *g, simulated_annealing *sa, int u)
{
    if (sa->hitting_set[u])
        return;

    sa->hitting_set[u] = 1;
    sa->cost++;

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        sa->cover_count[e]++;

        if (sa->cover_count[e] > 2)
            continue;

        int v = sa->one_tight[e];
        sa->score[v]--;

        assert(v != u);
        assert(sa->hitting_set[v]);
    }
}

void simulated_annealing_remove_vertex(graph_csr *g, simulated_annealing *sa, int u)
{
    if (!sa->hitting_set[u])
        return;

    if (sa->score[u] > 0)
    {
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            int d = g->V[g->n + e + 1] - g->V[g->n + e];
            if (sa->cover_count[e] > 1 || d > 2)
                continue;

            int v = g->E[g->V[g->n + e]];
            if (v == u)
                v = g->E[g->V[g->n + e] + 1];

            simulated_annealing_add_vertex(g, sa, v);
        }
        for (int i = g->V[u]; i < g->V[u + 1]; i++)
        {
            int e = g->E[i];
            int d = g->V[g->n + e + 1] - g->V[g->n + e];
            if (sa->cover_count[e] > 1)
                continue;

            int v = g->E[g->V[g->n + e] + (rand_r(&sa->seed) % d)];
            while (v == u)
                v = g->E[g->V[g->n + e] + (rand_r(&sa->seed) % d)];

            simulated_annealing_add_vertex(g, sa, v);
        }
    }

    assert(sa->score[u] == 0);

    sa->hitting_set[u] = 0;
    sa->cost--;

    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        sa->cover_count[e]--;

        if (sa->cover_count[e] > 1)
            continue;

        assert(sa->cover_count[e] == 1);

        for (int j = g->V[g->n + e]; j < g->V[g->n + e + 1]; j++)
        {
            int v = g->E[j];

            if (!sa->hitting_set[v])
                continue;

            sa->one_tight[e] = v;
            sa->score[v]++;

            break;
        }
    }
}

void simulated_annealing_shuffle(int *list, int n, unsigned int *seed)
{
    for (int i = 0; i < n - 1; i++)
    {
        int j = i + (rand_r(seed) % (n - i));
        int t = list[j];
        list[j] = list[i];
        list[i] = t;
    }
}

int simulated_annealing_one_two_swap(graph_csr *g, simulated_annealing *sa, int u, int *T)
{
    assert(!sa->hitting_set[u]);

    int n = 0;
    for (int i = g->V[u]; i < g->V[u + 1]; i++)
    {
        int e = g->E[i];
        sa->cover_count[e]++;
        if (sa->cover_count[e] > 2)
            continue;

        int v = sa->one_tight[e];
        sa->score[v]--;
        if (sa->score[v] == 0)
            T[n++] = v;
    }

    int found = 0, w1, w2;
    if (n >= 2)
    {
        // Found all remove candidates
        simulated_annealing_shuffle(T, n, &sa->seed);
        if (n > 32)
            n = 32;

        // Try every pair of vertiecs
        for (int i = 0; i < n && !found; i++)
        {
            int v1 = T[i];
            for (int j = i + 1; j < n && !found; j++)
            {
                int v2 = T[j];

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
                        if (sa->cover_count[g->E[i1]] == 2)
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
        sa->cover_count[e]--;
        if (sa->cover_count[e] > 1)
            continue;

        int v = sa->one_tight[e];
        sa->score[v]++;
    }

    if (found)
    {
        simulated_annealing_add_vertex(g, sa, u);
        simulated_annealing_remove_vertex(g, sa, w1);
        simulated_annealing_remove_vertex(g, sa, w2);
        return 1;
    }
    return 0;
}

void simulated_annealing_start(graph_csr *g, simulated_annealing *sa, double tl, volatile sig_atomic_t *tle, int offset, int verbose)
{
    double start = sa_get_wtime();

    for (long long k = 0; k < sa->k; k++)
    {
        double t = 1.0 - ((double)(k + 1) / (double)sa->k);
        t /= 4.0;
        int u = rand_r(&sa->seed) % g->n;

        if (sa->hitting_set[u] && sa->score[u] == 0)
        {
            simulated_annealing_remove_vertex(g, sa, u);
        }
        else if (sa->hitting_set[u])
        {
            if (rand_r(&sa->seed) <= exp(-(sa->score[u] - 1.0) / t) * (double)RAND_MAX)
            {
                simulated_annealing_remove_vertex(g, sa, u);
            }
        }

        if (sa->cost < sa->best_cost)
        {
            sa->time = sa_get_wtime() - sa->time_ref;
            sa->best_cost = sa->cost;
            for (int i = 0; i < g->n; i++)
                sa->best_hitting_set[i] = sa->hitting_set[i];
        }

        if ((k % 1028) == 0)
        {
            if (verbose)
            {
                printf("\r%10lld %10d %10.3lf %10d %10.3lf", k, sa->best_cost + offset, sa->time, sa->cost + offset, t);
                fflush(stdout);
            }

            double elapsed = sa_get_wtime() - start;
            if (elapsed > tl || *tle)
                break;
        }

        if (t < 0.08)
            break;
    }
    if (verbose)
        printf("\n");
}