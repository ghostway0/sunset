#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "internal/btree.h"
#include "sunset/bitmask.h"

typedef struct InputState {
    Bitmask keys;
} InputState;

typedef struct InputBinding {
    BTree btree;
} InputBinding;

typedef struct InputBindingCursor {
    BTreeNode *curr;
} InputBindingCursor;

void inputbinding_init(InputBinding *binding_out);

void inputbinding_add(InputBinding *binding, Bitmask comb);

InputBindingCursor inputbinding_query(
        InputBinding const *binding, InputState const *instate);
