#include <string.h>

#include "sunset/byte_stream.h"
#include "sunset/errors.h"
#include "sunset/vector.h"
#include "sunset/vfs.h"

int byte_stream_read_vector(
        struct byte_stream *stream, size_t size, vector(uint8_t) * out) {
    if (stream->cursor + size >= stream->size) {
        return ERROR_OUT_OF_BOUNDS;
    }

    size_t initial_size = vector_size(out);

    vector_resize(*out, initial_size + size);
    memcpy(out + initial_size, stream->data + stream->cursor, size);
    stream->cursor += size;

    return 0;
}

void byte_stream_read_until(
        struct byte_stream *stream, uint8_t delimeter, vector(uint8_t) * out) {
    // and we hope the compiler vectorizes this (without lto
    // it probably can't), but it doesn't matter much
    while (!byte_stream_is_eof(stream)) {
        uint8_t byte = stream->data[stream->cursor];

        if (byte == delimeter) {
            return;
        }

        vector_append(*out, byte);
        stream->cursor++;
    }
}

int byte_stream_skip(struct byte_stream *stream, size_t num_bytes) {
    // skipping the last byte is okay; after that it is not.
    if (byte_stream_is_eof(stream)) {
        return ERROR_OUT_OF_BOUNDS;
    }

    stream->cursor += num_bytes;

    return 0;
}

int byte_stream_read_raw(struct byte_stream *stream, size_t size, void *out) {
    if (stream->cursor + size >= stream->size) {
        return ERROR_OUT_OF_BOUNDS;
    }

    memcpy(out, stream->data + stream->cursor, size);
    stream->cursor += size;

    return 0;
}

void byte_stream_from_data(
        uint8_t const *data, size_t size, struct byte_stream *stream_out) {
    stream_out->data = data;
    stream_out->size = size;
    stream_out->cursor = 0;
}

bool byte_stream_is_eof(struct byte_stream const *stream) {
    return stream->cursor >= stream->size;
}
