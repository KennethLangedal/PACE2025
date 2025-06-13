#include "hypergraph.h"
#include "ipamir.h"
#include "mwis_reductions.h"
#include "maxsat.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int is_neighbor(graph *g, node_id u, node_id v) {
    // Binary search for neighbor v in u's adjacency list
    node_id *neighbors = g->V[u];
    long long lo = 0, hi = g->D[u] - 1;
    while (lo <= hi) {
        long long mid = (lo + hi) / 2;
        if (neighbors[mid] == v) return 1;
        if (neighbors[mid] < v) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0;
}

int grow_clique(graph *g, node_id start, int *used, node_id *clique, int max_size) {
    int size = 0;
    clique[size++] = start;
    used[start] = 1;

    for (int i = 0; i < g->D[start]; i++) {
        node_id cand = g->V[start][i];
        if (used[cand] || !g->A[cand]) continue;

        // Check if candidate is connected to all current clique members
        int connected = 1;
        for (int j = 0; j < size; j++) {
            if (!is_neighbor(g, cand, clique[j])) {
                connected = 0;
                break;
            }
        }

        if (connected) {
            clique[size++] = cand;
            used[cand] = 1;
            if (size >= max_size) break;
        }
    }

    return size;
}

clique_set *find_cliques(graph *g, int max_cliques, int max_clique_size) {
    int *used = calloc(g->n, sizeof(int));
    node_id *buffer = malloc(g->n * sizeof(node_id));

    clique_set *result = malloc(sizeof(clique_set));
    result->num_cliques = 0;
    result->sizes = malloc(max_cliques * sizeof(int));
    result->cliques = malloc(max_cliques * sizeof(node_id*));
    result->in_clique = malloc(g->n * sizeof(char*));
    for (node_id u = 0; u < g->n; u++) {
        result->in_clique[u] = calloc(g->D[u], sizeof(char));
    }


    for (node_id i = 0; i < g->n && result->num_cliques < max_cliques; i++) {
        if (!g->A[i] || used[i]) continue;

        int size = grow_clique(g, i, used, buffer, max_clique_size);
        if (size > 2) {
            // Copy clique to heap
            node_id *clique_copy = malloc(size * sizeof(node_id));
            memcpy(clique_copy, buffer, size * sizeof(node_id));
            result->cliques[result->num_cliques] = clique_copy;
            result->sizes[result->num_cliques] = size;

            // mark edges as in clique
            for (int x = 0; x < size; x++) {
                node_id u = clique_copy[x];
                for (int y = x + 1; y < size; y++) {
                    node_id v = clique_copy[y];

                    // Mark (u, v)
                    for (long long k = 0; k < g->D[u]; k++) {
                        if (g->V[u][k] == v) {
                            result->in_clique[u][k] = 1;
                            break;
                        }
                    }

                    // Mark (v, u)
                    for (long long k = 0; k < g->D[v]; k++) {
                        if (g->V[v][k] == u) {
                            result->in_clique[v][k] = 1;
                            break;
                        }
                    }
                }
            }

            result->num_cliques++;
        }
    }

    free(used);
    free(buffer);
    return result;
}

void free_clique_set(clique_set *cs) {
    if (!cs) return;
    for (int i = 0; i < cs->num_cliques; i++) {
        free(cs->cliques[i]);
    }
    free(cs->cliques);
    free(cs->sizes);
    free(cs);
}

long long maxsat_solve_hitting_set(hypergraph *hg, int **res) {
    if (!hg) return -1;

    void *sv = ipamir_init();

    // Add one soft clause per vertex
    for (int i = 0; i < hg->n; i++) {
        if (hg->Vd[i] == 0) continue;  // skip removed vertices

        ipamir_add_soft_lit(sv, (int32_t)(i + 1), 1);
    }

    // Add one hard clause per hyperedge
    for (int e = 0; e < hg->m; e++) {
        if (hg->Ed[e] == 0) continue;  // skip removed edges

        for (int j = 0; j < hg->Ed[e]; j++) {
            int v = hg->E[e][j];
            ipamir_add_hard(sv, (int32_t)(v + 1));
        }
        ipamir_add_hard(sv, 0);  // terminate clause
    }

    int result = ipamir_solve(sv);
    long long hs_sol = -1;

    if (result == 30) {  // 30 means OPTIMUM_FOUND
        uint64_t obj = ipamir_val_obj(sv);
        hs_sol = (long long)obj;

        int res_size = 0;
        *res = (int *) malloc(hs_sol * sizeof(int));

        // printf("%lld\n", hs_sol);
        
        for (int i = 0; i < hg->n; i++) {
            if (hg->Vd[i] == 0) continue;

            int32_t lit = (int32_t)(i + 1);
            int32_t val = ipamir_val_lit(sv, lit);
            if (val == lit)
                (*res)[res_size++] = lit - 1 ;
                // printf("%d\n", lit);
        }
    }

    ipamir_release(sv);

    return hs_sol;
}

long long maxsat_solve_hitting_set_implicit(hypergraph *hg, int **res) {
    if (!hg) return -1;

    void *sv = ipamir_init();

    // Add small edges
    for (int e = 0; e < hg->m; e++) {
        if (hg->Ed[e] > 0 && hg->Ed[e] <= 4) {
            for (int j = 0; j < hg->Ed[e]; j++) {
                int v = hg->E[e][j];
                ipamir_add_hard(sv, (int32_t)(v + 1));
            }
            ipamir_add_hard(sv, 0);
        }
    }

    // Add one soft clause per vertex
    for (int i = 0; i < hg->n; i++) {
        if (hg->Vd[i] == 0) continue;
        ipamir_add_soft_lit(sv, (int32_t)(i + 1), 1);
    }

    /* int result = ipamir_solve(sv);
    printf("result:%d", result); */

    int *edges_to_add = (int *)malloc(hg->m * sizeof(int));
    int edge_queue_size = 0;

    long long hs_sol = -1;
    int *solution = NULL;

    int counter = 0;
    while (1) {
        int result = ipamir_solve(sv);
        if (result != 30) break;

        uint64_t obj = ipamir_val_obj(sv);
        hs_sol = (long long)obj;

        free(solution);
        solution = (int *)malloc(hs_sol * sizeof(int));
        int *I = (int *)calloc(hg->n, sizeof(int));
        int res_size = 0;

        for (int i = 0; i < hg->n; i++) {
            if (hg->Vd[i] == 0) continue;
            int lit = i + 1;
            if (ipamir_val_lit(sv, lit) == lit) {
                I[i] = 1;
                solution[res_size++] = i;
            }
        }

        // Check for hyperedges not yet hit
        edge_queue_size = 0; // reset for next
        for (int e = 0; e < hg->m; e++) {
            if (hg->Ed[e] == 0) continue;

            // Skip small edges since they are already added and should be hit
            if (hg->Ed[e] <= 4) continue;

            int hit = 0;
            for (int j = 0; j < hg->Ed[e]; j++) {
                int u = hg->E[e][j];
                if (I[u]) {
                    hit = 1;
                    break;
                }
            }

            if (!hit) {
                edges_to_add[edge_queue_size++] = e;
            }
        }

        free(I);

        // Add only the new edges found to be not hit
        if (edge_queue_size == 0)
            break;

        for (int i = 0; i < edge_queue_size; i++) {
            int e = edges_to_add[i];
            for (int j = 0; j < hg->Ed[e]; j++) {
                int v = hg->E[e][j];
                ipamir_add_hard(sv, (int32_t)(v + 1));
            }
            ipamir_add_hard(sv, 0);
        }
    }

    if (hs_sol >= 0) {
        *res = solution;
    } else {
        *res = NULL;
        free(solution);
    }

    ipamir_release(sv);
    free(edges_to_add);

    return hs_sol;
}

int* maxsat_solve_MWIS(graph *g) {
    if (!g) return NULL;

    void *sv = ipamir_init();

    for (node_id i = 0; i < g->n; i++) {
        if (g->A[i] == 0) continue; // Skip inactive nodes

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

int* maxsat_solve_MWIS_with_cliques(graph *g, clique_set *cs) {
    if (!g) return NULL;

    void *sv = ipamir_init();

    // Add soft literals
    for (node_id i = 0; i < g->n; i++) {
        if (g->A[i] == 0) continue;
        ipamir_add_soft_lit(sv, -(int32_t)(i + 1), g->W[i]);
    }

    // Add clique-based hard constraints
    for (int c = 0; c < cs->num_cliques; c++) {
        if (cs->sizes[c] < 3) continue;

        // Add clause: (-v1 ∨ -v2 ∨ ... ∨ -vk)
        for (int j = 0; j < cs->sizes[c]; j++) {
            ipamir_add_hard(sv, -(int32_t)(cs->cliques[c][j] + 1));
        }
        ipamir_add_hard(sv, 0); // End clause
    }

    // Add pairwise constraints only if not in a clique
    for (node_id i = 0; i < g->n; i++) {
        if (!g->A[i]) continue;

        for (int j = 0; j < g->D[i]; j++) {
            node_id v = g->V[i][j];
            if (v <= i || !g->A[v]) continue;

            if (cs->in_clique[i][j]) continue;

            // Add clause: (-i ∨ -v)
            ipamir_add_hard(sv, -(int32_t)(i + 1));
            ipamir_add_hard(sv, -(int32_t)(v + 1));
            ipamir_add_hard(sv, 0);
        }
    }

    // Solve and extract solution
    int *solution = (int *)calloc(g->n, sizeof(int));
    int result = ipamir_solve(sv);

    if (result == 30) { // OPTIMUM_FOUND
        for (node_id i = 0; i < g->n; i++) {
            if (!g->A[i]) continue;
            int32_t lit = (int32_t)(i + 1);
            int32_t val = ipamir_val_lit(sv, lit);
            if (val == lit)
                solution[i] = 1;
        }
    }

    ipamir_release(sv);
    return solution;
}