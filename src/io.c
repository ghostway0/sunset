#include <stddef.h>
#include <stdint.h>

#include "sunset/math.h"
#include "sunset/utils.h"
#include "sunset/vfs.h"

#include "sunset/io.h"

ssize_t reader_read_until(
        Reader *reader, uint8_t delimiter, vector(uint8_t) * out) {
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

ssize_t reader_read(Reader *reader, size_t count, void *out) {
    return reader->read(reader->ctx, count, out);
}

void reader_skip(Reader *reader, size_t num_bytes) {
    size_t const buffer_size = 4096;

    uint8_t buffer[buffer_size];
    size_t remaining = num_bytes;

    while (remaining > 0) {
        size_t read =
                reader_read(reader, min(buffer_size, remaining), buffer);

        // FIXME: what should I do here?
        if (read <= 0) {
            return;
        }

        remaining -= read;
    }
}

ssize_t reader_read_to_vec(
        Reader *reader, size_t count, vector(uint8_t) * out) {
    size_t initial_size = vector_size(out);

    vector_resize(*out, initial_size + count);
    return reader_read(reader, count, out);
}

void reader_init(Reader *reader, void *ctx, ReadFn read) {
    reader->ctx = ctx;
    reader->read = read;
}

void writer_init(Writer *writer, void *ctx, WriteFn write) {
    writer->ctx = ctx;
    writer->write = write;
}

ssize_t writer_write(Writer *writer, void const *buf, size_t count) {
    return writer->write(writer->ctx, buf, count);
}

Writer *get_stdout(void) {
    static Writer stdout_writer;
    static bool initialized = false;

    if (!initialized) {
        static VfsFile stdout_file;
        vfs_get_stdout(&stdout_file);
        stdout_writer = vfs_file_writer(&stdout_file);
    }

    return &stdout_writer;
}

Reader *get_stdin(void) {
    static Reader stdout_reader;
    static bool initialized = false;

    if (!initialized) {
        static VfsFile stdin_file;
        vfs_get_stdin(&stdin_file);
        stdout_reader = vfs_file_reader(&stdin_file);
    }

    return &stdout_reader;
}
