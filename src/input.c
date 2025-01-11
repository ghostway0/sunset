#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "events.h"
#include "internal/btree.h"
#include "internal/math.h"
#include "internal/utils.h"
#include "sunset/bitmask.h"

#include "sunset/input.h"

typedef struct SetNode {
    Bitmask mask;
    BTreeNode node;
    EventId event_id;
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

    if (bitmask_is_superset(&right_node->mask, &left_node->mask)) {
        return ORDER_LESS_THAN;
    }

    if (bitmask_is_eql(&left_node->mask, &right_node->mask)) {
        return ORDER_EQUAL;
    }

    uint64_t left_hash = bitmask_hash(&left_node->mask);
    uint64_t right_hash = bitmask_hash(&right_node->mask);

    return left_hash < right_hash ? ORDER_LESS_THAN : ORDER_GREATER_THAN;
}

void inputbinding_init(EventQueue *event_queue, InputBinding *binding_out) {
    btree_init(compare_setnodes, NULL, NULL, &binding_out->btree);
    binding_out->event_queue = event_queue;
}

void inputbinding_destroy(InputBinding *binding) {
    btree_destroy(&binding->btree, setnode_destroy);
}

void binding_add(
        InputBinding *binding, Bitmask const *comb, EventId event_id) {
    SetNode *new_node = sunset_malloc(sizeof(SetNode));
    new_node->mask = bitmask_clone(comb);
    new_node->event_id = event_id;

    btree_insert(&binding->btree, &new_node->node);
}

bool binding_query(
        InputBinding const *binding, InputState const *input_state) {
    BTreeNode *it = binding->btree.root;

    while (!btree_node_is_sentinel(it)) {
        SetNode *current_node = container_of(it, SetNode, node);

        if (bitmask_is_superset(&current_node->mask, &input_state->keys)) {
            break;
        } else if (bitmask_is_superset(
                           &current_node->mask, &input_state->keys)) {
            it = btree_iter_gt(it);
        }
    }

    while (!btree_node_is_sentinel(it)) {
        SetNode *node = container_of(it, SetNode, node);

        event_queue_push(
                binding->event_queue, (Event){.event_id = node->event_id});

        it = btree_iter_gt(it);
    }

    return true;
}
