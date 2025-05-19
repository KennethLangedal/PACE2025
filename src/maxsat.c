#include "hypergraph.h"
#include "ipamir.h"
#include "mwis_reductions.h"

#include <stdio.h>
#include <stdint.h>

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
        
        /* for (int i = 0; i < hg->n; i++) {
            if (hg->Vd[i] == 0) continue;

            int32_t lit = (int32_t)(i + 1);
            int32_t val = ipamir_val_lit(sv, lit);
            if (val == lit)
                printf("Literal %d: TRUE\n", lit);
            else if (val == -lit)
                printf("Literal %d: FALSE\n", lit);
            else
                printf("Literal %d: UNDEFINED/IRRELEVANT\n", lit);
        }*/
    }

    ipamir_release(sv);

    return hs_sol;
}

long long maxsat_solve_MWIS(graph *g, long long offset) {
    if (!g) return -1;

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

    int result = ipamir_solve(sv);

    long long hs_sol = -1;

    if (result == 30) { // OPTIMUM_FOUND
        uint64_t obj = ipamir_val_obj(sv);

        /* printf("Total weight of active vertices: %llu\n", (unsigned long long)total_weight);
        printf("Objective value from solver: %llu\n", (unsigned long long)obj);
        printf("Computed result: %lld - (%llu - %llu) = %lld\n",
               offset, (unsigned long long)total_weight, (unsigned long long)obj,
               offset - (long long)(total_weight - obj)); */

        hs_sol = offset - (long long)(total_weight - obj);

        // printf("Hitting Set Solution: %lld\n", hs_sol);

        /* for (node_id i = 0; i < gr->n; i++) {
            if (gr->A[i] == 0) continue;

            int32_t lit = (int32_t)(i + 1);
            int32_t val = ipamir_val_lit(sv, lit);
            if (val == lit)
                printf("Literal %d: TRUE\n", lit);
            else if (val == -lit)
                printf("Literal %d: FALSE\n", lit);
            else
                printf("Literal %d: UNDEFINED/IRRELEVANT\n", lit);
        } */
    }

    ipamir_release(sv);
    return hs_sol;
}