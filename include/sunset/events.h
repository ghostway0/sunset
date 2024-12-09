#pragma once

#include <pthread.h>
#include <stdint.h>

#include <cglm/types.h>

#include "sunset/geometry.h"
#include "sunset/vector.h"

struct context;
struct event;

typedef void (*event_handler)(struct context *context, struct event event);

struct event_queue {
    vector(struct event) events;
    /// maps event type to a Vector of event handlers
    vector(vector(event_handler)) handlers;

    pthread_mutex_t *lock;
};

enum collision_type {
    COLLISION_ENTER_COLLIDER,
    COLLISION_EXIT_COLLIDER,
    COLLISION_REGULAR,
};

struct collision_event {
    vec3 a_velocity;
    struct object *a;
    vec3 b_velocity;
    struct object *b;

    enum collision_type type;
};

static_assert(
        sizeof(struct collision_event) <= 60, "collision_event too large");

struct event {
    uint32_t type_id;
    union {
        struct collision_event collision;
        struct point mouse_move;
        uint8_t other[60];
    } data;
};

void event_queue_init(struct event_queue *queue);

void event_queue_destroy(struct event_queue *queue);

void event_queue_add_handler(
        struct event_queue *queue, uint32_t type_id, event_handler handler);

void event_queue_push(struct event_queue *queue, struct event const event);

void event_queue_process(struct context *context, struct event_queue *queue);

void event_queue_process_one(struct context *context,
        struct event_queue *queue,
        struct event const event);

int event_queue_pop(struct event_queue *queue, struct event *event);

size_t event_queue_remaining(struct event_queue const *queue);
