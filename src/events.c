#include <pthread.h>
#include <stdint.h>

#include "sunset/utils.h"
#include "sunset/vector.h"

#include "sunset/events.h"

void event_queue_init(struct event_queue *queue) {
    vector_init(queue->events);
    vector_init(queue->handlers);

    queue->lock = sunset_malloc(sizeof(pthread_mutex_t));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(queue->lock, &mutex_attr);
}

void event_queue_destroy(struct event_queue *queue) {
    vector_destroy(queue->events);

    for (size_t i = 0; i < vector_size(queue->handlers); i++) {
        vector_destroy(queue->handlers[i]);
    }

    vector_destroy(queue->handlers);

    pthread_mutex_destroy(queue->lock);
    free(queue->lock);
}

void event_queue_add_handler(struct event_queue *queue,
        uint32_t type_id,
        struct event_handler handler) {
    pthread_mutex_lock(queue->lock);

    if (type_id >= vector_size(queue->handlers)) {
        vector_resize(queue->handlers, type_id + 1);
    }

    if (queue->handlers[type_id] == NULL) {
        vector_init(queue->handlers[type_id]);
    }

    vector_append(queue->handlers[type_id], handler);

    pthread_mutex_unlock(queue->lock);
}

void event_queue_push(struct event_queue *queue, struct event const event) {
    pthread_mutex_lock(queue->lock);
    vector_append_copy(queue->events, event);
    pthread_mutex_unlock(queue->lock);
}

void event_queue_process(struct event_queue *queue, void *global_context) {
    pthread_mutex_lock(queue->lock);

    for (size_t i = 0; i < vector_size(queue->events); i++) {
        struct event event = queue->events[i];
        event_queue_process_one(global_context, queue, event);
    }

    vector_clear(queue->events);
    pthread_mutex_unlock(queue->lock);
}

void event_queue_process_one(void *global_context,
        struct event_queue *queue,
        struct event const event) {
    if (queue->handlers[event.type_id] == NULL) {
        return;
    }

    for (size_t j = 0; j < vector_size(queue->handlers[event.type_id]);
            j++) {
        struct event_handler handler = queue->handlers[event.type_id][j];

        assert(handler.handler_fn != NULL);
        handler.handler_fn(global_context, handler.local_context, event);
    }
}

int event_queue_pop(struct event_queue *queue, struct event *event) {
    pthread_mutex_lock(queue->lock);

    if (event_queue_remaining(queue) == 0) {
        pthread_mutex_unlock(queue->lock);
        return -1;
    }

    *event = vector_pop_back(queue->events);

    pthread_mutex_unlock(queue->lock);

    return 0;
}

size_t event_queue_remaining(struct event_queue const *queue) {
    pthread_mutex_lock(queue->lock);

    size_t remaining = vector_size(queue->events);

    pthread_mutex_unlock(queue->lock);
    return remaining;
}
