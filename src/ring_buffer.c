#include <stdlib.h>
#include <string.h>

#include "sunset/errors.h"
#include "sunset/ring_buffer.h"
#include "sunset/utils.h"

int ring_buffer_append(struct ring_buffer *ring_buffer, void const *data) {
    if (((ring_buffer->head + 1) & (ring_buffer->buffer_size - 1))
            == ring_buffer->tail) {
        return -ERROR_RINGBUFFER_PTR_OVERRUN;
    }

    memcpy((char *)ring_buffer->buffer
                    + ring_buffer->head * ring_buffer->element_size,
            data,
            ring_buffer->element_size);

    ring_buffer->head =
            (ring_buffer->head + 1) & (ring_buffer->buffer_size - 1);

    return 0;
}

int ring_buffer_pop(struct ring_buffer *ring_buffer, void *data_out) {
    if (ring_buffer->head == ring_buffer->tail) {
        return -ERROR_RINGBUFFER_PTR_OVERRUN;
    }

    memcpy(data_out,
            (char *)ring_buffer->buffer
                    + ring_buffer->tail * ring_buffer->element_size,
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
    ring_buffer->buffer = sunset_calloc(buffer_size, element_size);
    ring_buffer->element_size = element_size;

    assert(ring_buffer->buffer != NULL);
}

void ring_buffer_destroy(struct ring_buffer *ring_buffer) {
    free(ring_buffer->buffer);
}

void ring_buffer_clear(struct ring_buffer *ring_buffer) {
    ring_buffer->head = 0;
    ring_buffer->tail = 0;
}

void ring_buffer_pop_wait(
        struct ring_buffer *ring_buffer, size_t n, void *data_out) {
    while (((ring_buffer->head + n) & (ring_buffer->buffer_size - 1))
            > ring_buffer->tail) {
    }

    int err = ring_buffer_pop(ring_buffer, data_out);
    assert(err == 0);
}

bool ring_buffer_empty(struct ring_buffer *ring_buffer) {
    return ring_buffer->head == ring_buffer->tail;
}
