#include <stdint.h>

#include "log.h"
#include "sunset/vector.h"

typedef void (*event_handler)(void *data);

struct event_queue {
    vector(struct event) events;
    /// maps event type to a vector of event handlers
    vector(vector(event_handler)) handlers;
};

struct event {
    uint32_t type_id;
    void *data;
};

void event_queue_init(struct event_queue *queue) {
    vector_init(queue->events, struct event);
    vector_init(queue->handlers, event_handler);
}

void event_queue_free(struct event_queue *queue) {
    vector_free(queue->events);

    for (size_t i = 0; i < vector_size(queue->handlers); i++) {
        vector_free(queue->handlers[i]);
    }

    vector_free(queue->handlers);
}

void event_queue_add_handler(
        struct event_queue *queue, uint32_t type_id, event_handler handler) {
    if (type_id >= vector_size(queue->handlers)) {
        vector_resize(queue->handlers, type_id + 1);
    }

    vector_append(queue->handlers[type_id], handler);
}

void event_queue_push(struct event_queue *queue, struct event const *event) {
    vector_append(queue->events, *event);
}

void event_queue_process(struct event_queue *queue) {
    for (size_t i = 0; i < vector_size(queue->events); i++) {
        struct event event = queue->events[i];

        for (size_t j = 0; j < vector_size(queue->handlers[event.type_id]);
                j++) {
            assert(queue->handlers[event.type_id][j] != NULL);

            queue->handlers[event.type_id][j](event.data);
        }
    }

    vector_clear(queue->events);
}
