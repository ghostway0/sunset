#pragma once

#include <pthread.h>
#include <stdint.h>

#include <cglm/types.h>

#include "sunset/geometry.h"
#include "sunset/vector.h"

typedef struct EngineContext EngineContext;
typedef struct Event Event;

struct EventHandler {
    void *local_context;
    void (*handler_fn)(EngineContext *engine_context,
            void *local_context,
            Event event);
} typedef EventHandler;

enum {
    SYSEV_TICK,
    SYSEV_RENDER,
    SYSEV_COLLISION,
    SYSEV_MOUSE_MOVE,
    SYSEV_MOUSE_CLICK,
};

struct EventQueue {
    vector(Event) events;
    /// maps event type to a Vector of event handlers
    vector(vector(EventHandler)) handlers;

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

struct Event {
    uint32_t type_id;
    union {
        struct collision_event collision;
        struct mouse_move_event mouse_move;
        // TODO: enum
        char key_up;
        char key_down;
        uint8_t other[60];
    };
} typedef Event;

void event_queue_init(EventQueue *queue);

void event_queue_destroy(EventQueue *queue);

void event_queue_add_handler(
        EventQueue *queue, uint32_t type_id, EventHandler handler);

void event_queue_push(EventQueue *queue, Event const event);

void event_queue_process(EventQueue *queue, void *global_context);

void event_queue_process_one(
        void *global_context, EventQueue *queue, Event const event);

int event_queue_pop(EventQueue *queue, Event *event);

size_t event_queue_remaining(EventQueue const *queue);
