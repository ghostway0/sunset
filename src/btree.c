#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/math.h"
#include "internal/utils.h"

#include "internal/btree.h"

typedef enum Direction {
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
} Direction;

static BTreeNode sentinel = {
        .left = &sentinel,
        .right = &sentinel,
        .parent = &sentinel,
        .color = NODE_COLOR_NULL,
};

static Direction get_parent_direction(BTreeNode const *node) {
    if (node == &sentinel) {
        return DIRECTION_RIGHT;
    }

    return node == node->parent->right ? DIRECTION_LEFT : DIRECTION_RIGHT;
}

static BTreeNode *sibling(BTreeNode const *node) {
    return get_parent_direction(node) == DIRECTION_LEFT
            ? node->parent->right
            : node->parent->left;
}

static BTreeNode *inside_child(BTreeNode *node) {
    return get_parent_direction(node) == DIRECTION_LEFT ? node->right
                                                        : node->left;
}

static bool is_inside_child(BTreeNode *node) {
    return get_parent_direction(node) != get_parent_direction(node->parent);
}

static BTreeNode *outside_child(BTreeNode *node) {
    return get_parent_direction(node) == DIRECTION_LEFT ? node->left
                                                        : node->right;
}

static void set_child(
        BTree *btree, BTreeNode *parent, BTreeNode *child, Direction dir) {
    if (parent == &sentinel) {
        btree->root = child;
    } else {
        if (dir == DIRECTION_LEFT) {
            parent->left = child;
        } else {
            parent->right = child;
        }
    }

    if (child != &sentinel) {
        child->parent = parent;
    }
}

static BTreeNode *uncle(BTreeNode *node) {
    return get_parent_direction(node->parent) == DIRECTION_LEFT
            ? node->parent->parent->right
            : node->parent->parent->left;
}

static void rotate_up(BTree *btree, BTreeNode *node) {
    memswap(&node->color, &node->parent->color, sizeof(NodeColor));

    Direction dir = get_parent_direction(node);
    BTreeNode *parent = node->parent;

    set_child(btree, node->parent, inside_child(node), dir);

    set_child(btree,
            node->parent->parent,
            node,
            get_parent_direction(node->parent));

    set_child(btree,
            node,
            parent,
            dir == DIRECTION_LEFT ? DIRECTION_RIGHT : DIRECTION_LEFT);
}

static void restore_black_property(BTree *btree, BTreeNode *to_fix) {
    if (sibling(to_fix)->color == NODE_COLOR_RED) {
        rotate_up(btree, sibling(to_fix));
    }

    BTreeNode *sibling_node = sibling(to_fix);

    if (inside_child(sibling_node)->color != NODE_COLOR_RED
            && outside_child(sibling_node)->color != NODE_COLOR_RED) {
        sibling_node->color = NODE_COLOR_RED;

        if (to_fix->parent->color == NODE_COLOR_RED) {
            to_fix->parent->color = NODE_COLOR_BLACK;
        } else if (to_fix->parent != btree->root) {
            restore_black_property(btree, to_fix->parent);
        }
    } else {
        BTreeNode *nephew = outside_child(sibling_node);

        if (nephew->color != NODE_COLOR_RED) {
            rotate_up(btree, nephew);
        }

        rotate_up(btree, sibling_node);
        uncle(to_fix)->color = NODE_COLOR_BLACK;
    }
}

static bool violates_red_property(BTreeNode *node) {
    return node->color == NODE_COLOR_RED
            && node->parent->color == NODE_COLOR_RED;
}

static void restore_red_property(BTree *btree, BTreeNode *node) {
    if (node->parent == btree->root) {
        // case 1
        node->parent->color = NODE_COLOR_BLACK;
    } else if (uncle(node)->color == NODE_COLOR_RED) {
        // case 2
        node->parent->color = NODE_COLOR_BLACK;
        uncle(node)->color = NODE_COLOR_BLACK;
        node->parent->parent->color = NODE_COLOR_RED;

        if (violates_red_property(node->parent->parent)) {
            restore_red_property(btree, node->parent->parent);
        }
    } else {
        // case 3
        BTreeNode *to_rotate = node;

        if (is_inside_child(node)) {
            rotate_up(btree, node);
            to_rotate = outside_child(node);
        }

        rotate_up(btree, to_rotate->parent);
    }
}

static void destroy_recursive(BTreeNode *node, DestroyFn destroyer) {
    if (node == &sentinel) {
        return;
    }

    destroy_recursive(node->left, destroyer);
    destroy_recursive(node->right, destroyer);

    destroyer(node);
}

void btree_init(CompareFn compare,
        BTreeCallback insert_callback,
        BTreeCallback delete_callback,
        BTree *btree_out) {
    *btree_out = (BTree){
            .root = &sentinel,
            .insert_callback = insert_callback,
            .delete_callback = delete_callback,
            .compare = compare,
    };
}

void btree_destroy(BTree *btree, DestroyFn destroyer) {
    destroy_recursive(btree->root, destroyer);
    btree->root = NULL;
    btree->compare = NULL;
    btree->delete_callback = NULL;
    btree->insert_callback = NULL;
}

bool btree_node_is_sentinel(BTreeNode *node) {
    return node == &sentinel;
}

// btree takes ownership on `new_node` until it is deleted.
int btree_insert(BTree *btree, BTreeNode *new_node) {
    *new_node = (BTreeNode){
            .left = &sentinel,
            .right = &sentinel,
            .parent = &sentinel,
            .color = NODE_COLOR_RED,
    };

    if (btree->root == &sentinel) {
        new_node->color = NODE_COLOR_BLACK;
        btree->root = new_node;
        return 0;
    }

    BTreeNode *current = btree->root;
    BTreeNode *parent = NULL;

    while (current != &sentinel) {
        parent = current;
        Order compare_result = btree->compare(new_node, current);

        if (compare_result == ORDER_GREATER_THAN) {
            current = current->right;
        } else if (compare_result == ORDER_LESS_THAN) {
            current = current->left;
        } else {
            return -1;
        }
    }

    assert(parent != NULL);

    Order compare_result = btree->compare(new_node, parent);

    if (compare_result == ORDER_GREATER_THAN) {
        parent->right = new_node;
    } else if (compare_result == ORDER_LESS_THAN) {
        parent->left = new_node;
    } else {
        unreachable();
    }

    new_node->parent = parent;

    if (violates_red_property(new_node)) {
        restore_red_property(btree, new_node);
    }

    if (btree->insert_callback != NULL) {
        btree->insert_callback(new_node);
    }

    return 0;
}

/// neither parent nor children fields have to be initialized
BTreeNode *btree_find(BTree const *btree, BTreeNode *node) {
    BTreeNode *current = btree->root;

    while (current != &sentinel) {
        Order compare_result = btree->compare(node, current);

        if (compare_result == ORDER_GREATER_THAN) {
            current = current->right;
        } else if (compare_result == ORDER_LESS_THAN) {
            current = current->left;
        } else {
            return current;
        }
    }

    return NULL;
}

void btree_delete(BTree *btree, BTreeNode *node) {
    assert(node != &sentinel);

    if (node != btree->root && node->color != NODE_COLOR_RED) {
        node->color = NODE_COLOR_NULL;
        restore_black_property(btree, node);
    }

    BTreeNode *child = node->left != &sentinel ? node->left : node->right;

    set_child(btree, node->parent, child, get_parent_direction(node));

    if (btree->delete_callback != NULL) {
        btree->delete_callback(node->parent);
    }
}

BTreeNode *btree_iter_gt(BTreeNode *node) {
    return node->right;
}

BTreeNode *btree_iter_lt(BTreeNode *node) {
    return node->left;
}
