#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/btree.h"
#include "internal/math.h"
#include "sunset/bitmask.h"
#include "utils.h"

#include "sunset/input.h"

typedef struct SetNode {
    Bitmask mask;
    BTreeNode node;
    // action
} SetNode;

static void setnode_destroy(BTreeNode *node) {
    SetNode *setnode = container_of(node, SetNode, node);
    free(setnode);
}

// 1 if superset, 0 if equal, -1 if not.
static Order compare_setnodes(void const *left, void const *right) {
    SetNode *left_node = container_of(left, SetNode, node);
    SetNode *right_node = container_of(right, SetNode, node);

    if (bitmask_is_superset(&left_node->mask, &right_node->mask)) {
        return ORDER_GREATER_THAN;
    }

    if (bitmask_is_eql(&left_node->mask, &right_node->mask)) {
        return ORDER_EQUAL;
    }

    return ORDER_LESS_THAN;
}

void inputbinding_init(InputBinding *binding_out) {
    btree_init(compare_setnodes, NULL, NULL, &binding_out->btree);
}

void inputbinding_destroy(InputBinding *binding) {
    btree_destroy(&binding->btree, setnode_destroy);
}

void inputbinding_add(InputBinding *binding, Bitmask comb) {
    SetNode *new_node = sunset_malloc(sizeof(SetNode));
    new_node->mask = comb;

    btree_insert(&binding->btree, &new_node->node);
}

InputBindingCursor inputbinding_query(
        InputBinding const *binding, InputState const *instate) {
    SetNode set_node = {.mask = instate->keys, .node = {}};

    BTreeNode *node = btree_find(&binding->btree, &set_node.node);

    return (InputBindingCursor){.curr = node};
}
