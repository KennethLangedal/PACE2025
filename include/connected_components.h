#pragma once

#include <stdbool.h>

#include "hypergraph.h"

typedef struct {
    int key;
    int val;
} pair;

/**
 * Can translate old integers to new integers and vice versa.
 */
typedef struct
{
    int capacity;
    int size;
    pair *old;
    pair *new;
} translation_table;

/**
 * Frees the memory from the translation table.
 */
void translation_table_free(translation_table *tt);

/**
 * Sorts the translation table. Call this once before get_old or get_new.
 */
void sort(translation_table *tt);

/**
 * Returns the old value. Needs O(log n) time. Call sort once before using.
 */
int get_old(translation_table *tt, int new);

/**
 * Returns the new value. Needs O(log n) time. Call sort once before using.
 */
int get_new(translation_table *tt, int old);

/**
 * Find connected components in a hypergraph
 *
 * @param hg The original hypergraph.
 * @param n_hypergraphs The number of connected components.
 *
 * @return An array of hypergraphs. The caller is responsible for freeing the memory.
 */
hypergraph** find_connected_components(hypergraph *hg, int *n_hypergraphs, translation_table ***vertex_tt, translation_table ***edge_tt);

/**
 * Determines if the hg consists of multiple non-empty components, but not how many.
 *
 * @param hg The hypergraph.
 * @return True if the hypergraph consists of multiple non-empty components, false else.
 */
bool are_multiple_components(hypergraph *hg);
