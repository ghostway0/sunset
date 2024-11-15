#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct bitmap {
    size_t num_chunks;
    uint64_t *chunks;
};

void bitmap_init(size_t size, struct bitmap *bitmap_out);

void bitmap_init_full(size_t size, struct bitmap *bitmap_out);

void bitmap_init_empty(size_t size, struct bitmap *bitmap_out);

bool bitmap_is_set(struct bitmap const *bitmap, size_t index);

void bitmap_set(struct bitmap const *bitmap, size_t index);

size_t bitmap_ctz(struct bitmap const *bitmap);

void bitmap_resize(struct bitmap *bitmap, size_t new_size);
