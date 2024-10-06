#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/gfx.h"
#include "sunset/quadtree.h"

// split node into 4 quadrants recursively
static void split_node(struct quad_tree *tree, struct quad_node *node) {
    assert(tree != NULL);
    assert(node != NULL);

    if (node->depth >= tree->max_depth) {
        return;
    }

    void *children_data[4] = {NULL};
    tree->split(tree, node->data, children_data);

    for (size_t i = 0; i < 4; ++i) {
        struct rect bounds = rect_subdivide_i(node->bounds, i);
        node->children[i] = malloc(sizeof(struct quad_node));
        quad_node_init(
                node->children[i], node->depth + 1, children_data[i], bounds);

        if (tree->should_split(tree, node->children[i])) {
            split_node(tree, node->children[i]);
        }
    }
}

void quad_tree_create(size_t max_depth,
        size_t max_objects,
        bool (*should_split)(struct quad_tree *, struct quad_node *),
        void (*split)(struct quad_tree *, void *, void **),
        void (*destroy_data)(void *),
        void *node_data,
        struct rect root_bounds,
        struct quad_tree *tree_out) {
    assert(should_split != NULL);
    assert(split != NULL);
    assert(destroy_data != NULL);
    assert(tree_out != NULL);

    tree_out->max_depth = max_depth;
    tree_out->max_objects = max_objects;
    tree_out->should_split = should_split;
    tree_out->split = split;
    tree_out->destroy_data = destroy_data;

    tree_out->root = malloc(sizeof(struct quad_node));
    quad_node_init(tree_out->root, 0, node_data, root_bounds);

    if (tree_out->should_split(tree_out, tree_out->root)) {
        split_node(tree_out, tree_out->root);
    }
}

void quad_node_init(
        struct quad_node *node, size_t depth, void *data, struct rect bounds) {
    assert(node != NULL);

    node->depth = depth;
    node->data = data;
    node->bounds = bounds;

    memset(node->children, 0, sizeof(node->children));
}

static void destroy_node(struct quad_node *node, void (*destroy_data)(void *)) {
    assert(destroy_data != NULL);

    if (node == NULL) {
        return;
    }

    for (size_t i = 0; i < 4; ++i) {
        destroy_node(node->children[i], destroy_data);
    }

    if (node->data != NULL) {
        destroy_data(node->data);
    }

    free(node);
}

void quadtree_destroy(struct quad_tree *tree) {
    assert(tree != NULL);

    destroy_node(tree->root, tree->destroy_data);
}
