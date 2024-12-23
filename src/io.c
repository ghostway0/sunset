#include <stddef.h>
#include <stdint.h>

#include "sunset/math.h"
#include "sunset/utils.h"

#include "sunset/io.h"

ssize_t reader_read_until(
        struct reader *reader, uint8_t delimiter, vector(uint8_t) * out) {
    ssize_t total_read = 0;
    ssize_t bytes_read;

    while (true) {
        uint8_t curr_byte;

        if ((bytes_read = reader->read(reader->ctx, 1, &curr_byte)) == 0) {
            return total_read;
        }

        total_read += bytes_read;
        vector_append(*out, curr_byte);

        if (curr_byte == delimiter) {
            break;
        }
    }

    return total_read;
}

ssize_t reader_read(struct reader *reader, size_t count, void *out) {
    return reader->read(reader->ctx, count, out);
}

void reader_skip(struct reader *reader, size_t num_bytes) {
    size_t const buffer_size = 4096;

    uint8_t buffer[buffer_size];
    size_t remaining = num_bytes;

    while (remaining > 0) {
        size_t read = reader_read(reader, min(buffer_size, remaining), buffer);

        // FIXME: what should I do here?
        if (read <= 0) {
            return;
        }

        remaining -= read;
    }
}

ssize_t reader_read_to_vec(
        struct reader *reader, size_t count, vector(uint8_t) * out) {
    size_t initial_size = vector_size(out);

    vector_resize(*out, initial_size + count);
    return reader_read(reader, count, out);
}

void reader_init(struct reader *reader, void *ctx, read_fn read) {
    reader->ctx = ctx;
    reader->read = read;
}
