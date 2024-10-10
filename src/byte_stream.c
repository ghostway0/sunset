#include <string.h>

#include "sunset/byte_stream.h"
#include "sunset/errors.h"

int byte_stream_read_raw(struct byte_stream *stream, void *out, size_t size) {
    if (stream->cursor + size > stream->size) {
        return -ERROR_STREAM_OVERRUN;
    }

    memcpy(out, stream->data + stream->cursor, size);
    stream->cursor += size;

    return 0;
}

int byte_stream_write_raw(
        struct byte_stream *stream, void const *in, size_t size) {
    if (stream->cursor + size > stream->size) {
        return -ERROR_STREAM_OVERRUN;
    }

    memcpy(stream->data + stream->cursor, in, size);
    stream->cursor += size;

    return 0;
}
