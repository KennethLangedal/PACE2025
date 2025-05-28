#pragma once

#include "hypergraph.h"
#include "mwis_reductions.h"

typedef struct {
    int num_cliques;      // Total number of cliques found
    int *sizes;           // sizes[i] = size of clique i
    node_id **cliques;    // cliques[i] = array of node_ids in clique i
    char **in_clique;     // in_clique[u][i] = 1 if edge (u, V[u][i]) is in any clique
} clique_set;

int is_neighbor(graph *g, node_id u, node_id v);

int grow_clique(graph *g, node_id start, int *used, node_id *clique, int max_size);

clique_set *find_cliques(graph *g, int max_cliques, int max_clique_size);

void free_clique_set(clique_set *cs);

long long maxsat_solve_hitting_set(hypergraph *hg);

int* maxsat_solve_MWIS(graph *g);

int* maxsat_solve_MWIS_with_cliques(graph *g, clique_set *cs);