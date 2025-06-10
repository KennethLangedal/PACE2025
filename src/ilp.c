#include "ilp.h"
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include <stdio.h>
#include <stdlib.h>

#define SCIP_CALL_EXC(x)                                        \
    do {                                                        \
        SCIP_RETCODE retcode = (x);                             \
        if (retcode != SCIP_OKAY) {                             \
            fprintf(stderr, "SCIP error code %d\n", retcode);   \
            exit(EXIT_FAILURE);                                 \
        }                                                       \
    } while (0)

int solve_mwis_ilp(const graph *g, const clique_set *cliques, int *solution, double timeout) {
    SCIP *scip;
    SCIP_VAR **vars = NULL;

    SCIP_CALL_EXC(SCIPcreate(&scip));
    SCIP_CALL_EXC(SCIPincludeDefaultPlugins(scip));
    SCIPmessagehdlrSetQuiet(SCIPgetMessagehdlr(scip), TRUE);
    SCIP_CALL_EXC(SCIPcreateProbBasic(scip, "mwis"));
    SCIP_CALL_EXC(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));
    SCIP_CALL_EXC(SCIPsetRealParam(scip, "limits/time", timeout));

    vars = (SCIP_VAR **)malloc(g->n * sizeof(SCIP_VAR *));
    if (!vars) {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }

    // Create binary variables for active nodes
    for (node_id i = 0; i < g->n; ++i) {
        if (g->A[i] == 0) {
            vars[i] = NULL;
            continue;
        }

        char name[32];
        snprintf(name, sizeof(name), "x%d", i);
        SCIP_CALL_EXC(SCIPcreateVarBasic(scip, &vars[i], name, 0.0, 1.0, g->W[i], SCIP_VARTYPE_BINARY));
        SCIP_CALL_EXC(SCIPaddVar(scip, vars[i]));
    }

    // 1. Add clique-based constraints
    for (int c = 0; c < cliques->num_cliques; ++c) {
        SCIP_CONS *cons;
        char cname[64];
        snprintf(cname, sizeof(cname), "clique_%d", c);

        SCIP_CALL_EXC(SCIPcreateConsBasicLinear(scip, &cons, cname, 0, NULL, NULL, -SCIPinfinity(scip), 1.0));

        for (int j = 0; j < cliques->sizes[c]; ++j) {
            node_id u = cliques->cliques[c][j];
            if (g->A[u] && vars[u]) {
                SCIP_CALL_EXC(SCIPaddCoefLinear(scip, cons, vars[u], 1.0));
            }
        }

        SCIP_CALL_EXC(SCIPaddCons(scip, cons));
        SCIP_CALL_EXC(SCIPreleaseCons(scip, &cons));
    }

    // 2. Add individual edge constraints only if not covered by a clique
    for (node_id i = 0; i < g->n; ++i) {
        if (!g->A[i]) continue;

        for (int j = 0; j < g->D[i]; ++j) {
            node_id v = g->V[i][j];

            if (i < v && g->A[v]) {
                // Check if the edge is already covered by a clique
                if (cliques->in_clique && cliques->in_clique[i] && cliques->in_clique[i][j])
                    continue;

                SCIP_CONS *cons;
                char cname[64];
                snprintf(cname, sizeof(cname), "conflict_%d_%d", i, v);

                SCIP_CALL_EXC(SCIPcreateConsBasicLinear(scip, &cons, cname, 0, NULL, NULL, -SCIPinfinity(scip), 1.0));
                SCIP_CALL_EXC(SCIPaddCoefLinear(scip, cons, vars[i], 1.0));
                SCIP_CALL_EXC(SCIPaddCoefLinear(scip, cons, vars[v], 1.0));
                SCIP_CALL_EXC(SCIPaddCons(scip, cons));
                SCIP_CALL_EXC(SCIPreleaseCons(scip, &cons));
            }
        }
    }

    SCIP_CALL_EXC(SCIPsolve(scip));

    SCIP_SOL *sol = SCIPgetBestSol(scip);
    int status = (SCIPgetStatus(scip) == SCIP_STATUS_OPTIMAL) ? 1 : 0;

    for (node_id i = 0; i < g->n; ++i) {
        if (g->A[i] && vars[i]) {
            solution[i] = (SCIPgetSolVal(scip, sol, vars[i]) > 0.5) ? 1 : 0;
        } else {
            solution[i] = 0;
        }
    }

    for (node_id i = 0; i < g->n; ++i) {
        if (vars[i]) {
            SCIP_CALL_EXC(SCIPreleaseVar(scip, &vars[i]));
        }
    }

    free(vars);
    SCIP_CALL_EXC(SCIPfree(&scip));
    return status;
}
