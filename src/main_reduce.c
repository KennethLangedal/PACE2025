#include "hypergraph.h"
#include "hs_reductions.h"
#include "mwis_reductions.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

static inline double get_wtime()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (double)tp.tv_sec + ((double)tp.tv_nsec / 1e9);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage ./REDUCE [time limit] < [graph]\n");
        return 0;
    }

    double t0 = get_wtime();

    hypergraph *hg = hypergraph_parse(stdin);

    hypergraph_sort(hg);

    int rc = 1;
    double tl = atof(argv[1]);
    double tl_hs = 60.0 > tl ? tl : 60.0;
    while (rc > 0 && tl > (get_wtime() - t0))
    {
        rc = 0;
        rc += hs_reductions_degree_one_rule(hg, tl_hs - (get_wtime() - t0));
        rc += hs_reductions_edge_domination(hg, tl_hs - (get_wtime() - t0));
        rc += hs_reductions_vertex_domination(hg, tl_hs - (get_wtime() - t0));
    }
    rc = 1;
    while (rc > 0)
    {
        rc = 0;
        rc += hs_reductions_degree_one_rule(hg, 10.0);
    }

    int nr = 0, mr = 0, md = 0;
    for (int i = 0; i < hg->n; i++)
    {
        if (hg->Vd[i] >= 2)
            nr++;
    }
    for (int i = 0; i < hg->m; i++)
    {
        if (hg->Ed[i] >= 2)
            mr++;
        if (hg->Ed[i] > md)
            md = hg->Ed[i];
    }

    printf("c {\"V_hs_reductions\":%d,\"E_hs_reductions\":%d}\n", nr, mr);

    long long offset = 0;

    long long m = 0;
    for (int i = 0; i < hg->n; i++)
        m += hg->Vd[i];

    for (int i = 0; i < hg->m; i++)
        if (hg->Ed[i] > 2)
            m += (hg->Ed[i] * hg->Ed[i]) / 2;

    int *FM = malloc(sizeof(int) * hg->n);
    if ((nr > 5000 || md == 2) && md < 32 && m < 5000000)
    {
        graph *gr = hs_reductions_to_mwis(hg, FM, 32, &offset);

        void *rd = mwis_reduction_run_struction(gr, tl - (get_wtime() - t0));

        printf("c {\"V_is_reductions\":%lld,\"E_is_reductions\":%lld}\n", gr->nr, gr->m);

        offset -= mwis_reduction_get_offset(rd);

        printf("c {\"offset\":%lld}\n", offset - gr->nr);

        int *FM_HS = malloc(sizeof(int) * gr->n);
        int id = 0;
        int m = 0;
        for (int i = 0; i < gr->n; i++)
        {
            FM_HS[i] = -1;
            if (!gr->A[i])
                continue;

            FM_HS[i] = id++;
            m += gr->D[i];

            if (gr->W[i] != 1)
            {
                printf("c Graph has weighted vertices, stopping\n");
                return 0;
            }
        }

        printf("p hs %lld %d\n", gr->nr, m / 2);

        for (int u = 0; u < gr->n; u++)
        {
            if (!gr->A[u])
                continue;

            for (int i = 0; i < gr->D[u]; i++)
            {
                int v = gr->V[u][i];
                if (u < v)
                    printf("%d %d\n", u + 1, v + 1);
            }
        }

        free(FM_HS);
    }
    else
    {
        offset = 0;
        int id = 0;
        for (int u = 0; u < hg->n; u++)
        {
            FM[u] = -1;
            if (hg->Vd[u] == 1)
                offset++;

            if (hg->Vd[u] <= 1)
                continue;

            FM[u] = id++;
        }

        printf("c {\"offset\":%lld}\n", offset);

        int m = 0;
        for (int e = 0; e < hg->m; e++)
        {
            if (hg->Ed[e] < 2)
                continue;

            m++;
        }

        printf("p hs %d %d\n", id, m);
        for (int e = 0; e < hg->m; e++)
        {
            if (hg->Ed[e] < 2)
                continue;

            for (int i = 0; i < hg->Ed[e]; i++)
            {
                int u = hg->E[e][i];
                printf("%d ", FM[u] + 1);
            }
            printf("\n");
        }
    }

    free(FM);
    hypergraph_free(hg);

    return 0;
}
