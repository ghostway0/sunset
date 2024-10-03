#include <stdlib.h>
#include <string.h>

#include "sunset/ring_buffer.h"

int ring_buffer_append(struct ring_buffer *ring_buffer, void const *data) {
    if (((ring_buffer->head + 1) & (ring_buffer->buffer_size))
            > ring_buffer->tail) {
        return -ERROR_PTR_OVERRUN;
    }

    memcpy(ring_buffer->buffer + ring_buffer->head * ring_buffer->element_size,
            data,
            ring_buffer->element_size);

    ring_buffer->head =
            (ring_buffer->head + 1) & (ring_buffer->buffer_size - 1);

    return 0;
}

int ring_buffer_pop(struct ring_buffer *ring_buffer, void *data_out) {
    if (((ring_buffer->tail + 1) & (ring_buffer->buffer_size - 1))
            > ring_buffer->head) {
        return -ERROR_PTR_OVERRUN;
    }

    memcpy(data_out,
            ring_buffer->buffer + ring_buffer->tail * ring_buffer->element_size,
            ring_buffer->element_size);

    ring_buffer->tail =
            (ring_buffer->tail + 1) & (ring_buffer->buffer_size - 1);

    return 0;
}

void ring_buffer_init(struct ring_buffer *ring_buffer,
        size_t buffer_size,
        size_t element_size) {
    assert(__builtin_popcount(buffer_size) == 1
            && "buffer_size must be power of two");

    ring_buffer->head = 0;
    ring_buffer->tail = 0;
    ring_buffer->buffer_size = buffer_size;
    ring_buffer->buffer = calloc(buffer_size, element_size);
    ring_buffer->element_size = element_size;

    assert(ring_buffer->buffer != NULL);
}

void ring_buffer_free(struct ring_buffer *ring_buffer) {
    free(ring_buffer->buffer);
}

void ring_buffer_clear(struct ring_buffer *ring_buffer) {
    ring_buffer->head = 0;
    ring_buffer->tail = 0;
}

void ring_buffer_pop_wait(
        struct ring_buffer *ring_buffer, size_t n, void *data_out) {
    while (((ring_buffer->head + n) & (ring_buffer->buffer_size))
            > ring_buffer->tail) {
    }

    int err = ring_buffer_pop(ring_buffer, data_out);
    assert(err == 0);
}
