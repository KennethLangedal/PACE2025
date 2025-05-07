#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include "chils.h"

#define MIN_CORE 512
#define DEFAULT_STEP_TIME 2.0
#define DEFAULT_STEP_COUNT LLONG_MAX

double chils_get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

chils *chils_init(graph_csr *g, int p, unsigned int seed)
{
    chils *c = malloc(sizeof(chils));

    c->p = p;
    c->step_time = DEFAULT_STEP_TIME;
    c->step_count = DEFAULT_STEP_COUNT;

    c->cost = 0;
    c->time = 0.0;

    c->LS = malloc(sizeof(local_search *) * p);
    c->LS_core = malloc(sizeof(local_search *) * p);

    c->d_core = malloc(sizeof(graph_csr));
    c->d_core->n = 0;
    c->d_core->V = malloc(sizeof(int) * (g->n + 1));
    c->d_core->V[0] = 0;
    c->d_core->E = malloc(sizeof(int) * g->V[g->n]);
    c->d_core->W = malloc(sizeof(long long) * g->n);

    c->FM = malloc(sizeof(int) * g->n);
    c->RM = malloc(sizeof(int) * g->n);
    c->A = malloc(sizeof(int) * g->n);

    for (int i = 0; i < p; i++)
    {
        c->LS[i] = local_search_init(g, seed + i);
        c->LS_core[i] = local_search_init(g, seed + p + i);
    }

    for (int i = 0; i < g->n; i++)
    {
        c->FM[i] = -1;
        c->RM[i] = -1;
        c->A[i] = 0;
    }

    return c;
}

void chils_free(chils *c)
{
    graph_csr_free(c->d_core);

    for (int i = 0; i < c->p; i++)
    {
        local_search_free(c->LS[i]);
        local_search_free(c->LS_core[i]);
    }

    free(c->LS);
    free(c->LS_core);

    free(c->FM);
    free(c->RM);
    free(c->A);

    free(c);
}

static inline int chils_find_overall_best(chils *c)
{
    int best = 0;
    for (int i = 1; i < c->p; i++)
        if (c->LS[i]->cost > c->LS[best]->cost ||
            (c->LS[i]->cost == c->LS[best]->cost && c->LS[i]->time < c->LS[best]->time))
            best = i;
    return best;
}

static inline int chils_find_first_best(chils *c)
{
    int best = 0;
    for (int i = 1; i < c->p; i++)
        if (c->LS[i]->cost > c->LS[best]->cost)
            best = i;
    return best;
}

static inline int chils_find_first_worst(chils *c)
{
    int worst = 0;
    for (int i = 1; i < c->p; i++)
        if (c->LS[i]->cost < c->LS[worst]->cost)
            worst = i;
    return worst;
}

void chils_print(chils *c, long long it, long long offset, double elapsed)
{
    int best = chils_find_overall_best(c), worst = chils_find_first_worst(c);
    printf("\r%6lld: %12lld (%3d %8.2lf) %12lld (%3d %8.2lf) %8.2lf %9d %9d",
           it, offset - c->LS[best]->cost, best, c->LS[best]->time,
           offset - c->LS[worst]->cost, worst, c->LS[worst]->time,
           elapsed, c->d_core->n, c->d_core->V[c->d_core->n]);
    fflush(stdout);
}

void chils_update_best(chils *c)
{
    int best = chils_find_overall_best(c);
    c->cost = c->LS[best]->cost;
    c->time = c->LS[best]->time;
}

void chils_run(graph_csr *g, chils *c, double tl, long long cl, long long offset, int verbose)
{
    double start = chils_get_wtime();
    double end = chils_get_wtime();
    double elapsed = end - start;

    if (verbose)
    {
        if (cl < LLONG_MAX)
            printf("Running chils for %.2lf seconds or %lld iterations\n", tl, cl);
        else
            printf("Running chils for %.2lf seconds\n", tl);
        printf("%7s %12s (%3s %8s) %12s (%3s %8s) %8s %9s %9s\n", "It.",
               "Best HS", "id", "time",
               "Worst HS", "id", "time",
               "time", "d-core V", "d-core E");
        chils_print(c, 0, offset, elapsed);
    }

    for (int i = 0; i < c->p; i++)
    {
        if (c->LS[i]->cost == 0 && i == 0)
            local_search_in_order_solution(g, c->LS[i]);
        else if (c->LS[i]->cost == 0)
            local_search_add_vertex(g, c->LS[i], rand_r(&c->LS[i]->seed) % g->n);

        local_search_greedy(g, c->LS[i]);
    }

    end = chils_get_wtime();
    elapsed = end - start;
    chils_update_best(c);
    if (verbose)
        chils_print(c, 0, offset, elapsed);

    int ci = 0;
    while (ci++ < cl && elapsed < tl)
    {
        /* Full graph LS */
        for (int i = 0; i < c->p; i++)
        {
            double remaining_time = tl - (chils_get_wtime() - start);
            double duration = c->step_time;
            if (remaining_time < duration)
                duration = remaining_time;
            if (duration > 0.0)
                local_search_explore(g, c->LS[i], duration, c->step_count, 0, 0);
        }

        /* Mark the D-core */
        for (int i = 0; i < g->n; i++)
        {
            int t = 0;
            for (int j = 0; j < c->p; j++)
                t += c->LS[j]->independent_set[i];
            c->A[i] = t > 0 && t < c->p;
        }

        /* Find the best solution */
        int best = chils_find_first_best(c);

        /* Construct the D-core */
        chils_update_best(c);
        if (verbose)
            chils_print(c, ci, offset, elapsed);

        graph_csr_subgraph(g, c->d_core, c->A, c->RM, c->FM);

        /* D-core LS */
        for (int i = 0; i < c->p; i++)
        {
            if (c->d_core->n == 0)
                continue;

            double remaining_time = tl - (chils_get_wtime() - start);
            double duration = c->step_time * 0.5;
            if (remaining_time < duration)
                duration = remaining_time;

            if (duration < 0.0)
                continue;

            local_search_reset(c->d_core, c->LS_core[i]);
            c->LS_core[i]->time_ref = c->LS[i]->time_ref;

            long long ref = 0;
            for (int u = 0; u < c->d_core->n; u++)
                if (c->LS[i]->independent_set[c->RM[u]])
                    ref += c->d_core->W[u];

            local_search_explore(c->d_core, c->LS_core[i], duration, c->step_count, 0, 0);

            if (ref <= c->LS_core[i]->cost || (i != best && (i % 2) == 0))
                for (int u = 0; u < c->d_core->n; u++)
                    if (c->LS_core[i]->independent_set[u] && !c->LS[i]->independent_set[c->RM[u]])
                        local_search_add_vertex(g, c->LS[i], c->RM[u]);

            if (ref < c->LS_core[i]->cost)
                c->LS[i]->time = c->LS_core[i]->time;
        }

        /* Find the best solution after LS on the CHILS core */
        best = chils_find_first_best(c);

        for (int i = 0; i < c->p; i++)
        {
            if (c->d_core->n < MIN_CORE && i != best && (i % 2) == 0)
                local_search_perturbe(g, c->LS[i]);
        }

        end = chils_get_wtime();
        elapsed = end - start;
        chils_update_best(c);
        if (verbose)
            chils_print(c, ci, offset, elapsed);
    }

    if (verbose)
        printf("\n");
}

void chils_set_solution(graph_csr *g, chils *c, const int *I)
{
    for (int i = 0; i < c->p; i++)
        for (int j = 0; j < g->n; j++)
            if (I[j])
                local_search_add_vertex(g, c->LS[i], j);
}

int *chils_get_best_independent_set(chils *c)
{
    int best = 0;
    for (int i = 1; i < c->p; i++)
        if (c->LS[i]->cost > c->LS[best]->cost)
            best = i;

    return c->LS[best]->independent_set;
}