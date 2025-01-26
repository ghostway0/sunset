#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "internal/mem_utils.h"
#include "sunset/geometry.h"
#include "sunset/octree.h"

// split node into 8 octants recursively
bool maybe_split_node(OcTree *tree, OcTreeNode *node) {
    if (!node->dirty) {
        return false;
    }

    if (node->depth >= tree->max_depth || !tree->should_split(tree, node)) {
        return false;
    }

    for (size_t i = 0; i < 8; ++i) {
        AABB bounds = aabb_subdivide_i(node->bounds, i, 8);
        node->children[i] = sunset_malloc(sizeof(OcTreeNode));

        void *data = tree->split_i(tree, node->data, bounds);

        octnode_init(
                node->depth + 1, data, node, bounds, node->children[i]);

        if (tree->should_split(tree, node->children[i])) {
            maybe_split_node(tree, node->children[i]);
        }
    }

    if (node->data != NULL && tree->destroy_data != NULL) {
        tree->destroy_data(node->data);
    }

    node->data = NULL;

    return true;
}

void octree_init(size_t max_depth,
        bool (*should_split)(OcTree *, OcTreeNode *),
        void *(*split)(OcTree *, void *, AABB bounds),
        void (*destroy_data)(void *),
        void *root_data,
        AABB root_bounds,
        OcTree *tree_out) {
    assert(should_split != NULL);
    assert(split != NULL);

    tree_out->max_depth = max_depth;
    tree_out->should_split = should_split;
    tree_out->split_i = split;
    tree_out->destroy_data = destroy_data;

    tree_out->root = sunset_malloc(sizeof(OcTreeNode));
    octnode_init(0, root_data, NULL, root_bounds, tree_out->root);

    maybe_split_node(tree_out, tree_out->root);
}

void octree_node_init(OcTreeNode *node,
        size_t depth,
        void *data,
        OcTreeNode *parent,
        AABB bounds) {
    assert(node != NULL);

    node->depth = depth;
    node->parent = parent;
    node->data = data;
    node->bounds = bounds;
    node->dirty = false;

    memset(node->children, 0, sizeof(node->children));
}

static void destroy_node(OcTreeNode *node, void (*destroy_data)(void *)) {
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

/// might split a node
void *octree_get_mutable(OcTree *tree, vec3 position) {
    assert(tree != NULL);

    OcTreeNode *current = tree->root;

    while (current != NULL) {
        OcTreeNode *next = NULL;

        // if the node is not split, we can return the node. otherwise,
        // we need to keep going down the children
        if (current->data && !maybe_split_node(tree, current)) {
            // the node can be changed by the user now, so we need to
            // monitor it next query. this is probably not the best way
            current->dirty = true;
            return current->data;
        }

        for (size_t i = 0; i < 8; ++i) {
            if (current->children[i] != NULL
                    && aabb_contains_point(
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

void *octree_query(OcTree const *tree, vec3 position) {
    assert(tree != NULL);

    OcTreeNode *current = tree->root;

    while (current != NULL) {
        OcTreeNode *next = NULL;

        if (current->data) {
            return current->data;
        }

        for (size_t i = 0; i < 8; ++i) {
            if (current->children[i] != NULL
                    && aabb_contains_point(
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

void octree_destroy(OcTree *tree) {
    assert(tree != NULL);

    destroy_node(tree->root, tree->destroy_data);
}

void *octree_init_resource(void) {
    OcTree *octree = sunset_malloc(sizeof(OcTree));

    return octree;
}
