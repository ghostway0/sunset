#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/utils.h"
#include "sunset/vector.h"

typedef ssize_t (*ReadFn)(void *ctx, size_t count, void *buf);

typedef struct Reader {
    void *ctx;
    ReadFn read;
} Reader;

void reader_init(Reader *reader, void *ctx, ReadFn read);

ssize_t reader_read(Reader *reader, size_t count, void *out);

ssize_t reader_read_until(
        Reader *reader, uint8_t delimiter, vector(uint8_t) * out);

void reader_skip(Reader *reader, size_t num_bytes);

ssize_t reader_read_to_vec(
        Reader *reader, size_t count, vector(uint8_t) * out);

typedef ssize_t (*WriteFn)(void *ctx, void const *buf, size_t count);

typedef struct Writer {
    void *ctx;
    WriteFn write;
} Writer;

void writer_init(Writer *writer, void *ctx, WriteFn write);

ssize_t writer_write(Writer *writer, void const *buf, size_t count);

ssize_t writer_write_byte(Writer *writer, uint8_t byte);

ssize_t writer_print_string(Writer *writer, char const *buf);
ssize_t writer_print_u64(Writer *writer, uint64_t value);
ssize_t writer_print_i64(Writer *writer, int64_t value);
ssize_t writer_print_f32(Writer *writer, float value);
ssize_t writer_print_f64(Writer *writer, double value);
ssize_t writer_vprintf(Writer *writer, char const *fmt, ...);

Writer *get_stdout(void);

Reader *get_stdin(void);
