#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

struct RingBuffer {
    _Atomic size_t head;
    _Atomic size_t tail;
    size_t element_size;
    /// power of two
    size_t buffer_size;
    void *buffer;
} typedef RingBuffer;

int ring_buffer_append(RingBuffer *ring_buffer, void const *data);

int ring_buffer_pop(RingBuffer *ring_buffer, void *data_out);

void ring_buffer_init(
        RingBuffer *ring_buffer, size_t buffer_size, size_t element_size);

void ring_buffer_destroy(RingBuffer *ring_buffer);

void ring_buffer_clear(RingBuffer *ring_buffer);

void ring_buffer_pop_wait(
        RingBuffer *ring_buffer, size_t n, void *data_out);

bool ring_buffer_empty(RingBuffer *ring_buffer);

#endif // RING_BUFFER_H
