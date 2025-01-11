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

enum SystemEvents {
    SYSTEM_EVENT_TICK,
    SYSTEM_EVENT_RENDER,
    SYSTEM_EVENT_COLLISION,
    SYSTEM_EVENT_MOUSE_MOVE,
    SYSTEM_EVENT_KEY_UP,
    SYSTEM_EVENT_KEY_DOWN,
    SYSTEM_EVENT_MOUSE_CLICK,
};

typedef struct EventQueue {
    vector(Event) events;
    /// maps event type to a Vector of event handlers
    vector(vector(EventHandler)) handlers;

    pthread_mutex_t *lock;
} EventQueue;

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

typedef struct MouseMoveEvent {
    struct point offset;
    struct point absolute;
} MouseMoveEvent;

typedef uint32_t EventId;

typedef struct Event {
    EventId event_id;
    uint8_t data[60];
} Event;

void event_queue_init(EventQueue *queue);

void event_queue_destroy(EventQueue *queue);

void event_queue_add_handler(
        EventQueue *queue, EventId event_id, EventHandler handler);

void event_queue_push(EventQueue *queue, Event const event);

void event_queue_process(EventQueue *queue, void *global_context);

void event_queue_process_one(
        void *global_context, EventQueue *queue, Event const event);

int event_queue_pop(EventQueue *queue, Event *event);

size_t event_queue_remaining(EventQueue const *queue);

#define events_push(__eq, __eid, ...)                                      \
    ({                                                                     \
        auto _tmp = __VA_ARGS__;                                           \
        Event __ev = {.event_id = (__eid)};                                \
        memcpy(__ev.data, &_tmp, sizeof(_tmp));                            \
        event_queue_push(__eq, __ev);                                      \
    })
