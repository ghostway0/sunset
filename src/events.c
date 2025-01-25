#include <pthread.h>
#include <stdint.h>

#include "internal/mem_utils.h"
#include "sunset/vector.h"

#include "sunset/events.h"

void event_queue_init(EventQueue *queue) {
    vector_init(queue->events);
    vector_init(queue->handlers);

    queue->lock = sunset_malloc(sizeof(pthread_mutex_t));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(queue->lock, &mutex_attr);
}

void event_queue_destroy(EventQueue *queue) {
    vector_destroy(queue->events);

    for (size_t i = 0; i < vector_size(queue->handlers); i++) {
        vector_destroy(queue->handlers[i]);
    }

    vector_destroy(queue->handlers);

    pthread_mutex_destroy(queue->lock);
    free(queue->lock);
}

void event_queue_add_handler(
        EventQueue *queue, EventId event_id, struct EventHandler handler) {
    pthread_mutex_lock(queue->lock);

    if (event_id >= vector_size(queue->handlers)) {
        vector_resize(queue->handlers, event_id + 1);
    }

    if (queue->handlers[event_id] == NULL) {
        vector_init(queue->handlers[event_id]);
    }

    vector_append(queue->handlers[event_id], handler);

    pthread_mutex_unlock(queue->lock);
}

void event_queue_push(EventQueue *queue, Event const event) {
    pthread_mutex_lock(queue->lock);
    vector_append_copy(queue->events, event);
    pthread_mutex_unlock(queue->lock);
}

void event_queue_process(EventQueue *queue, void *global_context) {
    pthread_mutex_lock(queue->lock);

    for (size_t i = 0; i < vector_size(queue->events); i++) {
        Event event = queue->events[i];
        event_queue_process_one(global_context, queue, event);
    }

    vector_clear(queue->events);
    pthread_mutex_unlock(queue->lock);
}

void event_queue_process_one(
        void *global_context, EventQueue *queue, Event const event) {
    pthread_mutex_lock(queue->lock);

    if (queue->handlers[event.event_id] == NULL) {
        return;
    }

    for (size_t j = 0; j < vector_size(queue->handlers[event.event_id]);
            j++) {
        EventHandler handler = queue->handlers[event.event_id][j];

        assert(handler.handler_fn != NULL);
        handler.handler_fn(global_context, handler.local_context, event);
    }

    pthread_mutex_unlock(queue->lock);
}

int event_queue_pop(EventQueue *queue, Event *event) {
    pthread_mutex_lock(queue->lock);

    if (event_queue_remaining(queue) == 0) {
        pthread_mutex_unlock(queue->lock);
        return -1;
    }

    *event = vector_pop_back(queue->events);

    pthread_mutex_unlock(queue->lock);

    return 0;
}

size_t event_queue_remaining(EventQueue const *queue) {
    pthread_mutex_lock(queue->lock);

    size_t remaining = vector_size(queue->events);

    pthread_mutex_unlock(queue->lock);
    return remaining;
}
