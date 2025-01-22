#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/vector.h"

typedef struct ByteStream {
    size_t size;
    uint8_t *data;
    size_t cursor;
    bool ro;
} ByteStream;

int bstream_read_vector(
        ByteStream *stream, size_t size, vector(uint8_t) * out);

ssize_t bstream_read(void *ctx, size_t count, void *out);

void bstream_from_ro(
        uint8_t const *data, size_t size, ByteStream *stream_out);

void bstream_from_rw(uint8_t *data, size_t size, ByteStream *stream_out);

bool bstream_is_eof(ByteStream const *stream);

int bstream_skip(ByteStream *stream, size_t num_bytes);

ssize_t bstream_write(ByteStream *stream, void const *buf, size_t size);
