#include "hypergraph.h"
#include "ipamir.h"
#include "mwis_reductions.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

long long maxsat_solve_hitting_set(hypergraph *hg) {
    if (!hg) return -1;

    void *sv = ipamir_init();

    // Add one hard clause per hyperedge
    for (int e = 0; e < hg->m; e++) {
        if (hg->Ed[e] == 0) continue;  // skip removed edges

        for (int j = 0; j < hg->Ed[e]; j++) {
            int v = hg->E[e][j];
            ipamir_add_hard(sv, (int32_t)(v + 1));
        }
        ipamir_add_hard(sv, 0);  // terminate clause
    }

    // Add one soft clause per vertex
    for (int i = 0; i < hg->n; i++) {
        if (hg->Vd[i] == 0) continue;  // skip removed vertices

        ipamir_add_soft_lit(sv, (int32_t)(i + 1), 1);
    }

    int result = ipamir_solve(sv);
    long long hs_sol = -1;

    if (result == 30) {  // 30 means OPTIMUM_FOUND
        uint64_t obj = ipamir_val_obj(sv);
        hs_sol = (long long)obj;

        printf("%lld\n", hs_sol);
        
        for (int i = 0; i < hg->n; i++) {
            if (hg->Vd[i] == 0) continue;

            int32_t lit = (int32_t)(i + 1);
            int32_t val = ipamir_val_lit(sv, lit);
            if (val == lit)
                printf("%d\n", lit);
        }
    }

    ipamir_release(sv);

    return hs_sol;
}

int* maxsat_solve_MWIS(graph *g) {
    if (!g) return NULL;

    void *sv = ipamir_init();

    uint64_t total_weight = 0;

    for (node_id i = 0; i < g->n; i++) {
        if (g->A[i] == 0) continue; // Skip inactive nodes

        total_weight += g->W[i];

        // Add soft literal with node weight
        ipamir_add_soft_lit(sv, -(int32_t)(i + 1), g->W[i]);

        for (int j = 0; j < g->D[i]; j++) {
            node_id v = g->V[i][j];
            if (v <= i || g->A[v] == 0) continue; // avoid duplicates and inactive

            // Add hard clause: (-i-1 ∨ -v-1)
            ipamir_add_hard(sv, -(int32_t)(i + 1));
            ipamir_add_hard(sv, -(int32_t)(v + 1));
            ipamir_add_hard(sv, 0); // terminate clause
        }
    }

    int *solution = (int *)malloc(sizeof(int) * g->n);
    for (node_id i = 0; i < g->n; i++)
        solution[i] = 0;

    int result = ipamir_solve(sv);

    if (result == 30) { // OPTIMUM_FOUND
        for (node_id i = 0; i < g->n; i++) {
            if (g->A[i] == 0) continue;

            int32_t lit = (int32_t)(i + 1);
            int32_t val = ipamir_val_lit(sv, lit);
            if (val == lit)
                solution[i] = 1;
        }
    }

    ipamir_release(sv);
    return solution;
}