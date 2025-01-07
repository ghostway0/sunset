#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "internal/utils.h"
#include "sunset/vector.h"

typedef ssize_t (*ReadFn)(void *ctx, size_t count, void *buf);

struct Reader {
    void *ctx;
    ReadFn read;
} typedef Reader;

void reader_init(Reader *reader, void *ctx, ReadFn read);

ssize_t reader_read(Reader *reader, size_t count, void *out);

ssize_t reader_read_until(
        Reader *reader, uint8_t delimiter, vector(uint8_t) * out);

#define reader_read_type(stream, out)                                      \
    do {                                                                   \
        int __err;                                                         \
        if ((__err = reader_read(stream, sizeof(*out), out))               \
                != sizeof(*out)) {                                         \
            return __err;                                                  \
        }                                                                  \
    } while (0)

void reader_skip(Reader *reader, size_t num_bytes);

ssize_t reader_read_to_vec(
        Reader *reader, size_t count, vector(uint8_t) * out);

typedef ssize_t (*WriteFn)(void *ctx, void const *buf, size_t count);

struct Writer {
    void *ctx;
    WriteFn write;
} typedef Writer;

void writer_init(Writer *writer, void *ctx, WriteFn write);

ssize_t writer_write(Writer *writer, void const *buf, size_t count);

#define writer_write_type(stream, in)                                      \
    do {                                                                   \
        int __err;                                                         \
        if ((__err = writer_write(stream, in, sizeof(*in)))                \
                != sizeof(*in)) {                                          \
            return __err;                                                  \
        }                                                                  \
    } while (0)

Writer *get_stdout(void);

Reader *get_stdin(void);
