#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "geometry.h"

#define DEFAULT_MAX_OCTREE_DEPTH 8

struct OcTreeNode {
    struct OcTreeNode *children[8];
    struct OcTreeNode *parent;
    AABB bounds;
    size_t depth;
    void *data;
    bool dirty;
} typedef OcTreeNode;

struct OcTree {
    struct OcTreeNode *root;
    size_t max_depth;

    bool (*should_split)(struct OcTree *tree, OcTreeNode *node);
    void *(*split_i)(struct OcTree *tree, void *data, AABB bounds);
    void (*destroy_data)(void *data);
} typedef OcTree;

void octr_init(size_t max_depth,
        bool (*should_split)(OcTree *, OcTreeNode *),
        void *(*split)(OcTree *, void *, AABB bounds),
        void (*destroy_data)(void *),
        void *node_data,
        AABB root_bounds,
        OcTree *tree_out);

void octr_destroy(OcTree *tree);

void octnode_init(size_t depth,
        void *data,
        OcTreeNode *parent,
        AABB bounds,
        OcTreeNode *node_out);

void *octr_query(OcTree const *tree, vec3 position);

void *octr_get_mutable(OcTree *tree, vec3 position);

// post-order traversal iterator
struct OcTreeIterator {
    OcTree *tree;
    OcTreeNode *current;
    size_t index;
} typedef OcTreeIterator;

struct ConstOcTreeIterator {
    OcTree const *tree;
    OcTreeNode const *current;
    size_t index;
} typedef ConstOcTreeIterator;

void octree_iterator_init(OcTree *tree, OcTreeIterator *iterator_out);

void octree_iterator_destroy(struct OcTreeIterator *iterator);

void *octree_iterator_next(OcTreeIterator *iterator);

void octree_const_iterator_destroy(ConstOcTreeIterator *iterator);

void octree_const_iterator_init(
        OcTree const *tree, ConstOcTreeIterator *iterator_out);

void *octree_const_iterator_next(ConstOcTreeIterator *iterator);
