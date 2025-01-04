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
    struct octree_node *root;
    size_t max_depth;

    bool (*should_split)(struct OcTree *tree, struct octree_node *node);
    void *(*split_i)(struct OcTree *tree, void *data, AABB bounds);
    void (*destroy_data)(void *data);
} typedef OcTree;

void octr_init(size_t max_depth,
        bool (*should_split)(OcTree *, struct octree_node *),
        void *(*split)(OcTree *, void *, AABB bounds),
        void (*destroy_data)(void *),
        void *node_data,
        AABB root_bounds,
        OcTree *tree_out);

void octr_destroy(OcTree *tree);

void octnode_init(size_t depth,
        void *data,
        struct octree_node *parent,
        AABB bounds,
        OcTreeNode *node_out);

void *octr_query(OcTree const *tree, vec3 position);

void *octr_get_mutable(OcTree *tree, vec3 position);

// post-order traversal iterator
struct octree_iterator {
    struct octree *tree;
    struct octree_node *current;
    size_t index;
};

struct const_octree_iterator {
    struct octree const *tree;
    struct octree_node const *current;
    size_t index;
};

void octree_iterator_init(
        struct octree *tree, struct octree_iterator *iterator_out);

void octree_iterator_destroy(struct octree_iterator *iterator);

void *octree_iterator_next(struct octree_iterator *iterator);

void octree_const_iterator_destroy(struct const_octree_iterator *iterator);

void octree_const_iterator_init(struct octree const *tree,
        struct const_octree_iterator *iterator_out);

void *octree_const_iterator_next(struct const_octree_iterator *iterator);
