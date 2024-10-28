#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

struct ring_buffer {
    _Atomic size_t head;
    _Atomic size_t tail;
    size_t element_size;
    /// power of two
    size_t buffer_size;
    void *buffer;
};

int ring_buffer_append(struct ring_buffer *ring_buffer, void const *data);

int ring_buffer_pop(struct ring_buffer *ring_buffer, void *data_out);

void ring_buffer_init(struct ring_buffer *ring_buffer,
        size_t buffer_size,
        size_t element_size);

void ring_buffer_free(struct ring_buffer *ring_buffer);

void ring_buffer_clear(struct ring_buffer *ring_buffer);

void ring_buffer_pop_wait(
        struct ring_buffer *ring_buffer, size_t n, void *data_out);

bool ring_buffer_empty(struct ring_buffer *ring_buffer);

#endif // RING_BUFFER_H
