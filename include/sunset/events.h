#pragma once

#include <stdint.h>

#include "vector.h"

typedef void (*event_handler)(void *data);

struct event_queue {
    vector(struct event) events;
    /// maps event type to a vector of event handlers
    vector(vector(event_handler)) handlers;
};

struct event {
    uint32_t type_id;
    uint8_t data[64];
};

void event_queue_init(struct event_queue *queue);

void event_queue_free(struct event_queue *queue);

void event_queue_add_handler(struct event_queue *queue, uint32_t type_id,
        event_handler handler);

void event_queue_push(struct event_queue *queue, struct event event);

void event_queue_process(struct event_queue *queue);
