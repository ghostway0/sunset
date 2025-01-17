#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/vector.h"

typedef struct ByteStream {
    size_t size;
    uint8_t *data;
    size_t cursor;
} ByteStream;

int byte_stream_read_vector(
        ByteStream *stream, size_t size, vector(uint8_t) * out);

ssize_t byte_stream_read(void *ctx, size_t count, void *out);

void byte_stream_from_buf(
        uint8_t *data, size_t size, ByteStream *stream_out);

bool byte_stream_is_eof(ByteStream const *stream);

int byte_stream_skip(ByteStream *stream, size_t num_bytes);

ssize_t byte_stream_write(ByteStream *stream, void const *buf, size_t size);
