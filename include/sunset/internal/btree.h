#ifndef SUNSET_BTREE_H_
#define SUNSET_BTREE_H_

#include "math.h"

typedef struct BTreeNode BTreeNode;

typedef Order (*CompareFn)(void const *, void const *);
typedef void (*BTreeCallback)(BTreeNode *);
typedef void (*DestroyFn)(BTreeNode *node);

typedef enum NodeColor {
    NODE_COLOR_NULL,
    NODE_COLOR_RED,
    NODE_COLOR_BLACK,
} NodeColor;

typedef struct BTreeNode {
    NodeColor color;
    struct BTreeNode *left;
    struct BTreeNode *right;
    struct BTreeNode *parent;
} BTreeNode;

typedef struct BTree {
    BTreeNode *root;

    CompareFn compare;
    BTreeCallback insert_callback;
    BTreeCallback delete_callback;
} BTree;

BTreeNode *btree_iter_gt(BTreeNode *node);
BTreeNode *btree_iter_lt(BTreeNode *node);

// btree takes ownership on `new_node`
int btree_insert(BTree *btree, BTreeNode *new_node);

/// neither parent nor children fields have to be initialized
BTreeNode *btree_find(BTree const *btree, BTreeNode *node);

void btree_delete(BTree *btree, BTreeNode *node);

bool btree_node_is_sentinel(BTreeNode *node);

void btree_init(CompareFn compare,
        BTreeCallback insert_callback,
        BTreeCallback delete_callback,
        BTree *btree_out);

void btree_destroy(BTree *btree, DestroyFn destroyer);

#endif // SUNSET_BTREE_H_
