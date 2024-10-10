#pragma once

#include <stddef.h>

struct byte_stream {
    size_t size;
    char const *data;
    size_t cursor;
};

int byte_stream_read_raw(struct byte_stream *stream, void *out, size_t size);

#define byte_stream_read(stream, type)                                         \
    ({                                                                         \
        type out;                                                              \
        int err = byte_stream_read_raw(stream, &out, sizeof(type));            \
        if (err) {                                                             \
            return err;                                                        \
        }                                                                      \
        out;                                                                   \
    })
