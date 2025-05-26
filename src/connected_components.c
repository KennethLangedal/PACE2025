#include "connected_components.h"

#include <stdlib.h>

void translation_table_free(translation_table *tt) {
    free(tt->old);
    free(tt->new);
}

// Comparison function for sorting by key
int compare_by_key(const void *a, const void *b) {
    const pair *pa = (const pair *) a;
    const pair *pb = (const pair *) b;
    return pa->key - pb->key;
}

void sort(translation_table *tt) {
    if (tt == NULL || tt->size <= 1)
        return;

    if (tt->old != NULL) {
        qsort(tt->old, tt->size, sizeof(pair), compare_by_key);
    }

    if (tt->new != NULL) {
        qsort(tt->new, tt->size, sizeof(pair), compare_by_key);
    }
}

int binary_search(pair *arr, int size, int key) {
    int left = 0, right = size - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (arr[mid].key == key) {
            return arr[mid].val;
        } else if (arr[mid].key < key) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return -1; // Key not found
}

int get_old(translation_table *tt, int new) {
    return binary_search(tt->new, tt->size, new);
}

int get_new(translation_table *tt, int old) {
    return binary_search(tt->old, tt->size, old);
}

hypergraph **find_connected_components(hypergraph *hg, int *n_hypergraphs, translation_table ***vertex_tt, translation_table ***edge_tt) {
    // Step 1: Determine the number of connected components and for each vertex
    // determine the corresponding component id
    int n_hg = 0;

    int      *component_id = (int *) malloc(hg->n * sizeof(int));
    for (int v             = 0; v < hg->n; ++v) { component_id[v] = -1; }

    int *bfs_arr     = (int *) malloc(hg->n * sizeof(int));
    int bfs_arr_size = 0;

    for (int v = 0; v < hg->n; ++v) {
        if (component_id[v] != -1) { continue; }

        int id = n_hg++;
        bfs_arr[bfs_arr_size++] = v;

        while (bfs_arr_size > 0) {
            int u = bfs_arr[--bfs_arr_size];
            // if(component_id[u] != -1 && component_id[u] != -2) { printf("Error\n"); exit(EXIT_FAILURE); }

            component_id[u] = id;

            for (int i = 0; i < hg->Vd[u]; ++i) {
                int e = hg->V[u][i];

                for (int j = 0; j < hg->Ed[e]; ++j) {
                    int e_v = hg->E[e][j];
                    if (component_id[e_v] == -1) {
                        component_id[e_v]       = -2;
                        bfs_arr[bfs_arr_size++] = e_v;
                    }
                }
            }
        }
    }

    // Step 2: Create each component
    hypergraph **components = (hypergraph **) malloc(n_hg * sizeof(hypergraph *));
    for (int   i            = 0; i < n_hg; ++i) {
        components[i] = (hypergraph *) malloc(sizeof(hypergraph));
        components[i]->n = 0;
        components[i]->m = 0;
    }

    // Step 3: Count the number of vertices and edges in each component
    for (int v = 0; v < hg->n; ++v) {
        components[component_id[v]]->n++;
    }
    for (int e = 0; e < hg->m; ++e) {
        if (hg->Ed[e] > 0)
            components[component_id[hg->E[e][0]]]->m++;
    }

    // Step 4: Allocate memory for vertices and edges
    for (int i = 0; i < n_hg; ++i) {
        components[i]->Vd = (int *) malloc(components[i]->n * sizeof(int));
        components[i]->Va = (int *) malloc(components[i]->n * sizeof(int));

        components[i]->Ed = (int *) malloc(components[i]->m * sizeof(int));
        components[i]->Ea = (int *) malloc(components[i]->m * sizeof(int));

        components[i]->V = (int **) malloc(components[i]->n * sizeof(int *));
        components[i]->E = (int **) malloc(components[i]->m * sizeof(int *));
    }

    // Step 5: Allocate memory for translation table
    translation_table **vertex_translation_tables = (translation_table **) malloc(n_hg * sizeof(translation_table*));
    translation_table **edge_translation_tables   = (translation_table **) malloc(n_hg * sizeof(translation_table*));

    for (int i = 0; i < n_hg; ++i) {
        vertex_translation_tables[i] = (translation_table *) malloc(sizeof(translation_table));
        vertex_translation_tables[i]->capacity = components[i]->n;
        vertex_translation_tables[i]->size     = 0;
        vertex_translation_tables[i]->old      = (pair *) malloc(components[i]->n * sizeof(pair));
        vertex_translation_tables[i]->new      = (pair *) malloc(components[i]->n * sizeof(pair));

        edge_translation_tables[i] = (translation_table *) malloc(sizeof(translation_table));
        edge_translation_tables[i]->capacity = components[i]->m;
        edge_translation_tables[i]->size     = 0;
        edge_translation_tables[i]->old      = (pair *) malloc(components[i]->m * sizeof(pair));
        edge_translation_tables[i]->new      = (pair *) malloc(components[i]->m * sizeof(pair));
    }

    // Step 6: Fill the vertex translation tables
    for (int old_v = 0; old_v < hg->n; ++old_v) {
        int id      = component_id[old_v];
        int tt_size = vertex_translation_tables[id]->size;
        int new_v   = tt_size == 0 ? 0 : vertex_translation_tables[id]->old[tt_size - 1].val + 1;

        vertex_translation_tables[id]->old[tt_size].key = old_v;
        vertex_translation_tables[id]->old[tt_size].val = new_v;
        vertex_translation_tables[id]->new[tt_size].key = new_v;
        vertex_translation_tables[id]->new[tt_size].val = old_v;
        vertex_translation_tables[id]->size++;
    }
    for (int i = 0; i < n_hg; ++i) {
        sort(vertex_translation_tables[i]);
    }

    // Step 7: Fill the edge translation table
    for (int old_e = 0; old_e < hg->m; ++old_e) {
        if (hg->Ed[old_e] > 0) {
            int id      = component_id[hg->E[old_e][0]];
            int tt_size = edge_translation_tables[id]->size;
            int new_e   = tt_size == 0 ? 0 : edge_translation_tables[id]->old[tt_size - 1].val + 1;

            edge_translation_tables[id]->old[tt_size].key = old_e;
            edge_translation_tables[id]->old[tt_size].val = new_e;
            edge_translation_tables[id]->new[tt_size].key = new_e;
            edge_translation_tables[id]->new[tt_size].val = old_e;
            edge_translation_tables[id]->size++;
        }
    }
    for (int i = 0; i < n_hg; ++i) {
        sort(edge_translation_tables[i]);
    }

    // Step 8: Create the hypergraphs
    for (int old_v = 0; old_v < hg->n; ++old_v) {
        int id    = component_id[old_v];
        int new_v = get_new(vertex_translation_tables[id], old_v);

        components[id]->Vd[new_v] = hg->Vd[old_v];
        components[id]->Va[new_v] = hg->Va[old_v];
        components[id]->V[new_v]  = (int *) malloc(components[id]->Vd[new_v] * sizeof(int));

        for (int i = 0; i < hg->Vd[old_v]; ++i) {
            int old_e = hg->V[old_v][i];
            int new_e = get_new(edge_translation_tables[id], old_e);

            components[id]->V[new_v][i] = new_e;
        }
    }

    for (int old_e = 0; old_e < hg->m; ++old_e) {
        if (hg->Ed[old_e] > 0) {
            int id = component_id[hg->E[old_e][0]];
            int new_e = get_new(edge_translation_tables[id], old_e);

            components[id]->Ed[new_e] = hg->Ed[old_e];
            components[id]->Ea[new_e] = hg->Ea[old_e];
            components[id]->E[new_e]  = (int *) malloc(components[id]->Ed[new_e] * sizeof(int));

            for (int i = 0; i < hg->Ed[old_e]; ++i) {
                int old_v = hg->E[old_e][i];
                int new_v = get_new(vertex_translation_tables[id], old_v);

                components[id]->E[new_e][i] = new_v;
            }
        }
    }

    // free temp memory
    free(component_id);
    free(bfs_arr);

    *n_hypergraphs = n_hg;
    *vertex_tt     = vertex_translation_tables;
    *edge_tt       = edge_translation_tables;

    return components;
}