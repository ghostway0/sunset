#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "geometry.h"

#define DEFAULT_MAX_OCTREE_DEPTH 8

struct octree_node {
    struct octree_node *children[8];
    struct octree_node *parent;
    struct aabb bounds;
    size_t depth;
    void *data;
    bool dirty;
};

struct octree {
    struct octree_node *root;
    size_t max_depth;

    bool (*should_split)(struct octree *tree, struct octree_node *node);
    void *(*split_i)(struct octree *tree, void *data, struct aabb bounds);
    void (*destroy_data)(void *data);
};

void octree_create(size_t max_depth,
        bool (*should_split)(struct octree *, struct octree_node *),
        void *(*split)(struct octree *, void *, struct aabb bounds),
        void (*destroy_data)(void *),
        void *node_data,
        struct aabb root_bounds,
        struct octree *tree_out);

void octree_destroy(struct octree *tree);

void octree_node_init(struct octree_node *node,
        size_t depth,
        void *data,
        struct octree_node *parent,
        struct aabb bounds);

void *octree_query(struct octree const *tree, vec3 position);

void *octree_get_mutable(struct octree *tree, vec3 position);

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
