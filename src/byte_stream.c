#include <stddef.h>
#include <string.h>

#include "internal/math.h"
#include "internal/mem_utils.h"
#include "sunset/byte_stream.h"
#include "sunset/errors.h"
#include "sunset/io.h"
#include "sunset/vector.h"

int byte_stream_read_vector(
        ByteStream *stream, size_t size, vector(uint8_t) * out) {
    if (stream->cursor + size >= stream->size) {
        return ERROR_OUT_OF_BOUNDS;
    }

    size_t initial_size = vector_size(out);

    vector_resize(*out, initial_size + size);
    memcpy(out + initial_size, stream->data + stream->cursor, size);
    stream->cursor += size;

    return 0;
}

int byte_stream_skip(ByteStream *stream, size_t num_bytes) {
    // skipping the last byte is okay; after that it is not.
    if (byte_stream_is_eof(stream)) {
        return ERROR_OUT_OF_BOUNDS;
    }

    stream->cursor += num_bytes;

    return 0;
}

ssize_t byte_stream_read(void *ctx, size_t count, void *buf) {
    ByteStream *stream = (ByteStream *)ctx;

    if (stream->cursor + count >= stream->size) {
        count = stream->size - stream->cursor;
    }

    memcpy(buf, stream->data + stream->cursor, count);
    stream->cursor += count;

    return count;
}

void byte_stream_from_buf(
        uint8_t *buf, size_t size, ByteStream *stream_out) {
    stream_out->data = buf;
    stream_out->size = size;
    stream_out->cursor = 0;
}

bool byte_stream_is_eof(ByteStream const *stream) {
    return stream->cursor >= stream->size;
}

ssize_t byte_stream_write(ByteStream *stream, void const *buf, size_t size) {
}
