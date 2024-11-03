#pragma once

#include <stdint.h>
#include <pthread.h>

#include "vector.h"

typedef void (*event_handler)(void *data);

struct event_queue {
    vector(struct event) events;
    /// maps event type to a vector of event handlers
    vector(vector(event_handler)) handlers;

    pthread_mutex_t *lock;
};

struct mouse_move_event {
    float x;
    float y;
};

struct collision_event {
    struct object *a;
    struct object *b;
};

struct event {
    uint32_t type_id;
    union {
        struct collision_event collision;
        struct mouse_move_event mouse_move;
        uint8_t other[60];
    } data;
};

void event_queue_init(struct event_queue *queue);

void event_queue_free(struct event_queue *queue);

void event_queue_add_handler(
        struct event_queue *queue, uint32_t type_id, event_handler handler);

void event_queue_push(struct event_queue *queue, struct event const event);

void event_queue_process(struct event_queue *queue);

int event_queue_pop(struct event_queue *queue, struct event *event);

size_t event_queue_remaining(struct event_queue const *queue);
