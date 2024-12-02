#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/vector.h"

struct vfs_file;

struct byte_stream {
    size_t size;
    // currently write streams are not supported.
    uint8_t const *data;
    size_t cursor;
};

int byte_stream_read_vector(
        struct byte_stream *stream, size_t size, vector(uint8_t) *out);

int byte_stream_read_raw(struct byte_stream *stream, size_t size, void *out);

#define byte_stream_read(stream, type, out)                                    \
    do {                                                                       \
        int __err;                                                             \
        if ((__err = byte_stream_read_raw(stream, sizeof(type), out))) {       \
            return __err;                                                      \
        }                                                                      \
    } while (0)

#define byte_stream_try_read(stream, type, out)                                \
    byte_stream_read_raw(stream, sizeof(type), out)

void byte_stream_from_data(
        uint8_t const *data, size_t size, struct byte_stream *stream_out);

int byte_stream_from_file(
        struct vfs_file *file, struct byte_stream *stream_out);

void byte_stream_read_until(
        struct byte_stream *stream, uint8_t delimeter, vector(uint8_t) *out);

bool byte_stream_is_eof(struct byte_stream const *stream);

int byte_stream_skip(struct byte_stream *stream, size_t num_bytes);
