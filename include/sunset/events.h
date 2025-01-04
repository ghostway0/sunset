#pragma once

#include <pthread.h>
#include <stdint.h>

#include <cglm/types.h>

#include "sunset/geometry.h"
#include "sunset/vector.h"

typedef struct EngineContext EngineContext;
struct event;

struct event_handler {
    void *local_context;
    void (*handler_fn)(EngineContext *engine_context,
            void *local_context,
            struct event event);
};

enum system_event {
    SYSTEM_EVENT_TICK,
    SYSTEM_EVENT_COLLISION,
    SYSTEM_EVENT_MOUSE_MOVE,
    SYSTEM_EVENT_MOUSE_CLICK,
    SYSTEM_EVENT_KEY_DOWN,
    SYSTEM_EVENT_KEY_UP,
};

struct EventQueue {
    vector(struct event) events;
    /// maps event type to a Vector of event handlers
    vector(vector(struct event_handler)) handlers;

    pthread_mutex_t *lock;
} typedef EventQueue;

enum collision_type {
    COLLISION_ENTER_COLLIDER,
    COLLISION_EXIT_COLLIDER,
    COLLISION_REGULAR,
};

struct collision_event {
    enum collision_type type;
    vec3 a_velocity;
    struct object *a;
    vec3 b_velocity;
    struct object *b;
};

static_assert(
        sizeof(struct collision_event) <= 60, "collision_event too large");

struct mouse_move_event {
    struct point offset;
    struct point absolute;
};

struct event {
    uint32_t type_id;
    union {
        struct collision_event collision;
        struct mouse_move_event mouse_move;
        // TODO: enum
        char key_up;
        char key_down;
        uint8_t other[60];
    };
};

void event_queue_init(EventQueue *queue);

void event_queue_destroy(EventQueue *queue);

void event_queue_add_handler(EventQueue *queue,
        uint32_t type_id,
        struct event_handler handler);

void event_queue_push(EventQueue *queue, struct event const event);

void event_queue_process(EventQueue *queue, void *global_context);

void event_queue_process_one(void *global_context,
        EventQueue *queue,
        struct event const event);

int event_queue_pop(EventQueue *queue, struct event *event);

size_t event_queue_remaining(EventQueue const *queue);
