#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/vector.h"

struct byte_stream {
    size_t size;
    // currently write streams are not supported.
    uint8_t const *data;
    size_t cursor;
};

int byte_stream_read_vector(
        struct byte_stream *stream, size_t size, vector(uint8_t) * out);

ssize_t byte_stream_read(void *ctx, size_t count, void *out);

void byte_stream_from_buf(
        uint8_t const *data, size_t size, struct byte_stream *stream_out);

bool byte_stream_is_eof(struct byte_stream const *stream);

int byte_stream_skip(struct byte_stream *stream, size_t num_bytes);
