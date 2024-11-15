#include <pthread.h>
#include <stdint.h>

#include "sunset/events.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

void event_queue_init(struct event_queue *queue) {
    vector_init(queue->events, struct event);
    vector_init(queue->handlers, event_handler);

    queue->lock = sunset_malloc(sizeof(pthread_mutex_t));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(queue->lock, &mutex_attr);
}

void event_queue_free(struct event_queue *queue) {
    vector_free(queue->events);

    for (size_t i = 0; i < vector_size(queue->handlers); i++) {
        vector_free(queue->handlers[i]);
    }

    vector_free(queue->handlers);

    pthread_mutex_destroy(queue->lock);
    free(queue->lock);
}

void event_queue_add_handler(
        struct event_queue *queue, uint32_t type_id, event_handler handler) {
    pthread_mutex_lock(queue->lock);

    if (type_id >= vector_size(queue->handlers)) {
        vector_resize(queue->handlers, type_id + 1);
    }

    vector_append(queue->handlers[type_id], handler);
    pthread_mutex_unlock(queue->lock);
}

void event_queue_push(struct event_queue *queue, struct event const event) {
    pthread_mutex_lock(queue->lock);
    vector_append_copy(queue->events, event);
    pthread_mutex_unlock(queue->lock);
}

void event_queue_process(struct event_queue *queue) {
    pthread_mutex_lock(queue->lock);

    for (size_t i = 0; i < vector_size(queue->events); i++) {
        struct event event = queue->events[i];

        for (size_t j = 0; j < vector_size(queue->handlers[event.type_id]);
                j++) {
            assert(queue->handlers[event.type_id][j] != NULL);

            queue->handlers[event.type_id][j](&event.data);
        }
    }

    vector_clear(queue->events);
    pthread_mutex_unlock(queue->lock);
}

int event_queue_pop(struct event_queue *queue, struct event *event) {
    pthread_mutex_lock(queue->lock);

    if (event_queue_remaining(queue) == 0) {
        pthread_mutex_unlock(queue->lock);
        return -1;
    }

    *event = vector_pop(queue->events);

    pthread_mutex_unlock(queue->lock);

    return 0;
}

size_t event_queue_remaining(struct event_queue const *queue) {
    pthread_mutex_lock(queue->lock);

    size_t remaining = vector_size(queue->events);

    pthread_mutex_unlock(queue->lock);
    return remaining;
}
