#include <stddef.h>
#include <stdint.h>

#include "internal/math.h"
#include "internal/utils.h"
#include "sunset/errors.h"
#include "sunset/vector.h"
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

ssize_t writer_print_string(Writer *writer, char const *buf) {
    return writer_write(writer, buf, strlen(buf));
}

ssize_t writer_write_byte(Writer *writer, uint8_t byte) {
    return writer_write(writer, &byte, 1);
}

ssize_t writer_print_u64(Writer *writer, uint64_t value) {
    if (value == 0) {
        return writer_write_byte(writer, '0');
    }

    char buffer[20];
    size_t i = sizeof(buffer);

    while (value) {
        buffer[--i] = '0' + (value % 10);
        value /= 10;
    }

    return writer->write(writer->ctx, buffer + i, sizeof(buffer) - i);
}

ssize_t writer_print_i64(Writer *writer, int64_t value) {
    if (value == 0) {
        return writer_write_byte(writer, '0');
    }

    bool negative = value < 0;

    if (negative) {
        if (writer_write_byte(writer, '-') != 1) {
            return ERROR_IO;
        }

        value = -value;
    }

    return writer_print_u64(writer, (uint64_t)value);
}

static ssize_t print_integer_part(Writer *writer, int64_t value) {
    if (value == 0) {
        return writer_write_byte(writer, '0');
    }

    char buffer[20];
    int i = sizeof(buffer);
    uint64_t abs_value = (value < 0) ? -value : value;

    while (abs_value) {
        buffer[--i] = '0' + (abs_value % 10);
        abs_value /= 10;
    }

    return writer_write(writer, buffer + i, sizeof(buffer) - i);
}

static ssize_t print_fractional_part(
        Writer *writer, double fractional_part, size_t precision) {
    if (writer_write_byte(writer, '.') != 1) {
        return ERROR_IO;
    }

    for (size_t i = 0; i < precision; ++i) {
        fractional_part *= 10;

        int64_t digit = fractional_part;

        if (writer_write_byte(writer, '0' + digit) != 1) {
            return ERROR_IO;
        }

        fractional_part -= digit;
        if (fractional_part == 0.0) {
            break;
        }
    }

    return precision;
}

ssize_t writer_print_f32(Writer *writer, float value) {
    if (value == 0.0f) {
        return writer_print_string(writer, "0.0");
    }

    if (value < 0) {
        if (writer_write_byte(writer, '-') != 1) {
            return ERROR_IO;
        }
        value = -value;
    }

    int64_t integer_part = (int64_t)value;
    double fractional_part = value - integer_part;

    ssize_t written_int = print_integer_part(writer, integer_part);

    if (written_int < 0) {
        return written_int;
    }

    ssize_t written_frac =
            print_fractional_part(writer, fractional_part, 6);

    if (written_frac < 0) {
        return written_frac;
    }

    return written_frac + written_int;
}

ssize_t writer_print_f64(Writer *writer, double value) {
    if (value == 0.0) {
        return writer_print_string(writer, "0.0");
    }

    if (value < 0) {
        if (writer_write_byte(writer, '-') != 1) {
            return ERROR_IO;
        }
        value = -value;
    }

    int64_t integer_part = (int64_t)value;
    double fractional_part = value - integer_part;

    ssize_t written = print_integer_part(writer, integer_part);
    if (written < 0)
        return written;

    written = print_fractional_part(writer, fractional_part, 15);

    return written > 0 ? 1 : written;
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
