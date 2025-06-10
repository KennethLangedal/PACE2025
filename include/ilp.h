#ifndef ILP_H
#define ILP_H

#include "mwis_reductions.h"
#include "maxsat.h"

/**
 * Solves MWIS on the given graph using SCIP.
 *
 * @param g Pointer to the graph
 * @param solution Output array of length g->n, where solution[i] = 1 if node i is selected
 * @param timeout Time limit in seconds
 * @return 1 if solved to optimality, 0 otherwise
 */
int solve_mwis_ilp(const graph *g, const clique_set *cliques, int *solution, double timeout);

#endif // ILP_H
