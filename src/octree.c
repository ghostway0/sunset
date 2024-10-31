#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/geometry.h"
#include "sunset/octree.h"

// split node into 8 octants recursively
static void split_node(struct oct_tree *tree, struct oct_node *node) {
    assert(tree != NULL);
    assert(node != NULL);

    if (node->depth >= tree->max_depth) {
        return;
    }

    for (size_t i = 0; i < 8; ++i) {
        struct box bounds = box_subdivide_i(node->bounds, i, 8);
        node->children[i] = malloc(sizeof(struct oct_node));

        void *data = tree->split_i(tree, node->data, bounds);

        oct_node_init(node->children[i], node->depth + 1, data, node, bounds);

        if (tree->should_split(tree, node->children[i])) {
            split_node(tree, node->children[i]);
        }
    }

    if (node->data != NULL && tree->destroy_data != NULL) {
        tree->destroy_data(node->data);
    }

    node->data = NULL;
}

void oct_tree_create(size_t max_depth,
        bool (*should_split)(struct oct_tree *, struct oct_node *),
        void *(*split)(struct oct_tree *, void *, struct box bounds),
        void (*destroy_data)(void *),
        void *node_data,
        struct box root_bounds,
        struct oct_tree *tree_out) {
    assert(should_split != NULL);
    assert(split != NULL);
    assert(tree_out != NULL);

    tree_out->max_depth = max_depth;
    tree_out->should_split = should_split;
    tree_out->split_i = split;
    tree_out->destroy_data = destroy_data;

    tree_out->root = malloc(sizeof(struct oct_node));
    oct_node_init(tree_out->root, 0, node_data, NULL, root_bounds);

    if (tree_out->should_split(tree_out, tree_out->root)) {
        split_node(tree_out, tree_out->root);
    }
}

void oct_node_init(struct oct_node *node,
        size_t depth,
        void *data,
        struct oct_node *parent,
        struct box bounds) {
    assert(node != NULL);

    node->depth = depth;
    node->parent = parent;
    node->data = data;
    node->bounds = bounds;

    memset(node->children, 0, sizeof(node->children));
}

static void destroy_node(struct oct_node *node, void (*destroy_data)(void *)) {
    if (node == NULL) {
        return;
    }

    for (size_t i = 0; i < 8; ++i) {
        destroy_node(node->children[i], destroy_data);
    }

    if (node->data != NULL && destroy_data != NULL) {
        destroy_data(node->data);
    }

    free(node);
}

void *oct_tree_query(struct oct_tree const *tree, vec3 position) {
    assert(tree != NULL);

    struct oct_node *current = tree->root;

    while (current != NULL) {
        struct oct_node *next = NULL;


        if (current->data) {
            return current->data;
        }

        for (size_t i = 0; i < 8; ++i) {
            if (current->children[i] != NULL
                    && box_contains_point(
                            current->children[i]->bounds, position)) {
                next = current->children[i];
                break;
            }
        }

        assert(next);
        current = next;
    }

    return NULL;
}

void octree_const_iterator_init(struct oct_tree const *tree,
        struct const_octree_iterator *iterator_out) {
    assert(tree != NULL);
    assert(iterator_out != NULL);

    iterator_out->tree = tree;
    iterator_out->current = tree->root;
    iterator_out->index = 0;
}

static void goto_first_leaf(struct octree_iterator *iterator) {
    while (iterator->current != NULL
            && iterator->current->children[0] != NULL) {
        iterator->current = iterator->current->children[0];
    }
}

void octree_iterator_init(
        struct oct_tree *tree, struct octree_iterator *iterator_out) {
    assert(tree != NULL);
    assert(iterator_out != NULL);

    iterator_out->tree = tree;
    iterator_out->current = tree->root;
    iterator_out->index = 0;

    goto_first_leaf(iterator_out);
}

void octree_iterator_destroy(struct octree_iterator *iterator) {
    free(iterator);
}

// iterate over all leafs
void *octree_iterator_next(struct octree_iterator *iterator) {
    assert(iterator != NULL);

    while (iterator->current != NULL) {
        if (iterator->index < 8) {
            struct oct_node *child =
                    iterator->current->children[iterator->index];

            if (child != NULL) {
                iterator->current = child;
                iterator->index = 0;
            } else {
                iterator->index++;
            }
        } else {
            void *data = iterator->current->data;

            while (iterator->current != NULL && iterator->index >= 8) {
                iterator->current = iterator->current->parent;
                iterator->index++;
            }

            return data;
        }
    }

    return NULL;
}

void oct_tree_destroy(struct oct_tree *tree) {
    assert(tree != NULL);

    destroy_node(tree->root, tree->destroy_data);
}

void octree_const_iterator_destroy(struct const_octree_iterator *iterator) {
    free(iterator);
}

void *octree_const_iterator_next(struct const_octree_iterator *iterator) {
    struct octree_iterator *mutable_iterator =
            (struct octree_iterator *)iterator;
    return octree_iterator_next(mutable_iterator);
}
