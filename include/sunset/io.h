#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sunset/utils.h"
#include "sunset/vector.h"

typedef ssize_t (*read_fn)(void *ctx, size_t count, void *buf);

struct reader {
    void *ctx;
    read_fn read;
};

void reader_init(struct reader *reader, void *ctx, read_fn read);

ssize_t reader_read(struct reader *reader, size_t count, void *out);

ssize_t reader_read_until(
        struct reader *reader, uint8_t delimiter, Vector(uint8_t) * out);

#define reader_read_type(stream, out)                                          \
    do {                                                                       \
        int __err;                                                             \
        if ((__err = reader_read(stream, sizeof(*out), out)) != sizeof(*out)) {        \
            return __err;                                                      \
        }                                                                      \
    } while (0)

void reader_skip(struct reader *reader, size_t num_bytes);

ssize_t reader_read_to_vec(
        struct reader *reader, size_t count, Vector(uint8_t) * out);
