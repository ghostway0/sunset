#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "geometry.h"

#define DEFAULT_MAX_OCTREE_DEPTH 8

struct oct_node {
    size_t depth;
    struct oct_node *children[8];
    struct aabb bounds;
    struct oct_node *parent;
    void *data;
};

struct oct_tree {
    struct oct_node *root;
    size_t max_depth;
    bool (*should_split)(struct oct_tree *tree, struct oct_node *node);
    void *(*split_i)(struct oct_tree *tree, void *data, struct aabb bounds);
    void (*destroy_data)(void *data);
};

void oct_tree_create(size_t max_depth,
        bool (*should_split)(struct oct_tree *, struct oct_node *),
        void *(*split)(struct oct_tree *, void *, struct aabb bounds),
        void (*destroy_data)(void *),
        void *node_data,
        struct aabb root_bounds,
        struct oct_tree *tree_out);

void oct_tree_destroy(struct oct_tree *tree);

void oct_node_init(struct oct_node *node,
        size_t depth,
        void *data,
        struct oct_node *parent,
        struct aabb bounds);

void *oct_tree_query(struct oct_tree const *tree, vec3 position);

// post-order traversal iterator
struct octree_iterator {
    struct oct_tree *tree;
    struct oct_node *current;
    size_t index;
};

struct const_octree_iterator {
    struct oct_tree const *tree;
    struct oct_node const *current;
    size_t index;
};

void octree_iterator_init(
        struct oct_tree *tree, struct octree_iterator *iterator_out);

void octree_iterator_destroy(struct octree_iterator *iterator);

void *octree_iterator_next(struct octree_iterator *iterator);

void octree_const_iterator_destroy(struct const_octree_iterator *iterator);

void octree_const_iterator_init(struct oct_tree const *tree,
        struct const_octree_iterator *iterator_out);

void *octree_const_iterator_next(struct const_octree_iterator *iterator);

void maybe_split_node(struct oct_tree *tree, struct oct_node *node);
