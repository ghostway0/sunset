#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "internal/btree.h"
#include "sunset/bitmask.h"
#include "sunset/events.h"

typedef struct InputState {
    Bitmask keys;
} InputState;

typedef struct InputBinding {
    BTree btree;
    EventQueue *event_queue;
} InputBinding;

void inputbinding_init(EventQueue *event_queue, InputBinding *binding_out);

void inputbinding_destroy(InputBinding *binding);

void binding_add(
        InputBinding *binding, Bitmask const *comb, EventId event_id);

bool binding_query(
        InputBinding const *binding, InputState const *input_state);
