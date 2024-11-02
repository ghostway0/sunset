#include <stdint.h>
#include <threads.h>

#include "sunset/events.h"
#include "sunset/vector.h"

void event_queue_init(struct event_queue *queue) {
    vector_init(queue->events, struct event);
    vector_init(queue->handlers, event_handler);

    queue->lock = malloc(sizeof(mtx_t));
    mtx_init(queue->lock, mtx_recursive);
}

void event_queue_free(struct event_queue *queue) {
    vector_free(queue->events);

    for (size_t i = 0; i < vector_size(queue->handlers); i++) {
        vector_free(queue->handlers[i]);
    }

    vector_free(queue->handlers);

    mtx_destroy(queue->lock);
}

void event_queue_add_handler(
        struct event_queue *queue, uint32_t type_id, event_handler handler) {
    mtx_lock(queue->lock);
    if (type_id >= vector_size(queue->handlers)) {
        vector_resize(queue->handlers, type_id + 1);
    }

    vector_append(queue->handlers[type_id], handler);
    mtx_unlock(queue->lock);
}

void event_queue_push(struct event_queue *queue, struct event const event) {
    mtx_lock(queue->lock);
    vector_append_copy(queue->events, event);
    mtx_unlock(queue->lock);
}

void event_queue_process(struct event_queue *queue) {
    mtx_lock(queue->lock);

    for (size_t i = 0; i < vector_size(queue->events); i++) {
        struct event event = queue->events[i];

        for (size_t j = 0; j < vector_size(queue->handlers[event.type_id]);
                j++) {
            assert(queue->handlers[event.type_id][j] != NULL);

            queue->handlers[event.type_id][j](&event.data);
        }
    }

    vector_clear(queue->events);
    mtx_unlock(queue->lock);
}

int event_queue_pop(struct event_queue *queue, struct event *event) {
    mtx_lock(queue->lock);

    if (event_queue_remaining(queue) == 0) {
        mtx_unlock(queue->lock);
        return -1;
    }

    *event = vector_pop(queue->events);

    mtx_unlock(queue->lock);

    return 0;
}

size_t event_queue_remaining(struct event_queue const *queue) {
    mtx_lock(queue->lock);

    size_t remaining = vector_size(queue->events);

    mtx_unlock(queue->lock);
    return remaining;
}
