#pragma once

#include <stdint.h>
#include <stddef.h>

struct byte_stream {
    size_t size;
    uint8_t *data;
    size_t cursor;
};

int byte_stream_read_raw(struct byte_stream *stream, void *out, size_t size);

#define byte_stream_read(stream, type, out)                                    \
    byte_stream_read_raw(stream, out, sizeof(type))

int byte_stream_from(const uint8_t *data, size_t size, struct byte_stream *stream_out);
