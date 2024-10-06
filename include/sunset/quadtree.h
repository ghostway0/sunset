#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "gfx.h"

struct quad_node {
    size_t depth;
    struct quad_node *children[4];
    struct rect bounds;
    void *data;
};

struct quad_tree {
    struct quad_node *root;
    size_t max_depth;
    size_t max_objects;
    bool (*should_split)(struct quad_tree *tree, struct quad_node *node);
    void (*split)(struct quad_tree *tree, void *data, void **children_data);
    void (*destroy_data)(void *data);
};

void quad_tree_destroy(struct quad_tree *tree);

void quad_node_init(
        struct quad_node *node, size_t depth, void *data, struct rect bounds);
