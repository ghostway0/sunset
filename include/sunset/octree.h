#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "geometry.h"

struct oct_node {
    size_t depth;
    struct oct_node *children[8];
    struct box bounds;
    void *data;
};

struct oct_tree {
    struct oct_node *root;
    size_t max_depth;
    size_t max_objects;
    bool (*should_split)(struct oct_tree *tree, struct oct_node *node);
    void *(*split_i)(struct oct_tree *tree, void *data, struct box bounds);
    void (*destroy_data)(void *data);
};

void oct_tree_create(size_t max_depth,
        size_t max_objects,
        bool (*should_split)(struct oct_tree *, struct oct_node *),
        void *(*split)(struct oct_tree *, void *, struct box bounds),
        void (*destroy_data)(void *),
        void *node_data,
        struct box root_bounds,
        struct oct_tree *tree_out);

void oct_tree_destroy(struct oct_tree *tree);

void oct_node_init(
        struct oct_node *node, size_t depth, void *data, struct box bounds);

void *oct_tree_query(struct oct_tree *tree, vec3 position);
