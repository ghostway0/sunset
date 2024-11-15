#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/utils.h"

struct bitmap {
    size_t num_chunks;
    uint64_t *chunks;
};

void bitmap_init(size_t size, struct bitmap *bitmap_out) {
    bitmap_out->num_chunks = (size + 63) / 64;
    bitmap_out->chunks =
            sunset_malloc(bitmap_out->num_chunks * sizeof(uint64_t));
}

void bitmap_init_full(size_t size, struct bitmap *bitmap_out) {
    bitmap_init(size, bitmap_out);
    memset(bitmap_out->chunks,
            UINT64_MAX,
            bitmap_out->num_chunks * sizeof(uint64_t));
}

void bitmap_init_empty(size_t size, struct bitmap *bitmap_out) {
    bitmap_init(size, bitmap_out);
    memset(bitmap_out->chunks, 0, bitmap_out->num_chunks * sizeof(uint64_t));
}

bool bitmap_is_set(const struct bitmap *bitmap, size_t index) {
    return bitmap->chunks[index / 64] & (1ULL << (index % 64));
}

void bitmap_set(struct bitmap *bitmap, size_t index) {
    bitmap->chunks[index / 64] |= (1ULL << (index % 64));
}

size_t bitmap_ctz(const struct bitmap *bitmap) {
    for (size_t i = 0; i < bitmap->num_chunks; ++i) {
        uint64_t chunk = bitmap->chunks[i];

        if (chunk != 0) {
            return i * 64 + __builtin_ctzll(chunk);
        }
    }

    return bitmap->num_chunks * 64;
}

void bitmap_resize(struct bitmap *bitmap, size_t new_size) {
    bitmap->num_chunks = (new_size + 63) / 64;
    bitmap->chunks =
            realloc(bitmap->chunks, bitmap->num_chunks * sizeof(uint64_t));
}
