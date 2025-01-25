#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "geometry.h"

#define DEFAULT_MAX_OCTREE_DEPTH 8

typedef struct OcTreeNode {
    struct OcTreeNode *children[8];
    struct OcTreeNode *parent;
    AABB bounds;
    size_t depth;
    void *data;
    bool dirty;
} OcTreeNode;

typedef struct OcTree {
    struct OcTreeNode *root;
    size_t max_depth;

    bool (*should_split)(struct OcTree *tree, OcTreeNode *node);
    void *(*split_i)(struct OcTree *tree, void *data, AABB bounds);
    void (*destroy_data)(void *data);
} OcTree;

void octree_init(size_t max_depth,
        bool (*should_split)(OcTree *, OcTreeNode *),
        void *(*split)(OcTree *, void *, AABB bounds),
        void (*destroy_data)(void *),
        void *node_data,
        AABB root_bounds,
        OcTree *tree_out);

void octree_destroy(OcTree *tree);

void octnode_init(size_t depth,
        void *data,
        OcTreeNode *parent,
        AABB bounds,
        OcTreeNode *node_out);

void *octr_query(OcTree const *tree, vec3 position);

void *octr_get_mutable(OcTree *tree, vec3 position);
