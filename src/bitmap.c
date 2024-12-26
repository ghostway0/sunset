#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/bitmap.h"
#include "sunset/utils.h"

void bitmap_init(size_t size, struct bitmap *bitmap_out) {
    bitmap_out->num_chunks = (size + 63) / 64;
    bitmap_out->chunks =
            sunset_malloc(bitmap_out->num_chunks * sizeof(uint64_t));
}

void bitmap_init_full(size_t size, struct bitmap *bitmap_out) {
    bitmap_init(size, bitmap_out);
    memset(bitmap_out->chunks,
            0xFFFFFFFF,
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

bool bitmap_is_eql(struct bitmap const *bitmap, struct bitmap const *other) {
    return memcmp(bitmap->chunks, other->chunks, bitmap->num_chunks * 8) == 0;
}

bool bitmap_is_superset(
        struct bitmap const *bitmap, struct bitmap const *other) {
    for (size_t i = 0; i < bitmap->num_chunks; ++i) {
        if ((bitmap->chunks[i] & other->chunks[i]) != other->chunks[i]) {
            return false;
        }
    }

    return true;
}

size_t bitmap_popcount(struct bitmap const *bitmap) {
    size_t popcount = 0;

    for (size_t i = 0; i < bitmap->num_chunks; ++i) {
        popcount += __builtin_popcountll(bitmap->chunks[i]);
    }

    return popcount;
}

void bitmap_lsb_reset(struct bitmap *bitmap) {
    for (size_t i = 0; i < bitmap->num_chunks; i--) {
        if (bitmap->chunks[i] != 0) {
            bitmap->chunks[i] &= bitmap->chunks[i] - 1;
            return;
        }
    }
}

// premature but vectorization is cool
bool bitmap_is_zero(struct bitmap const *bitmap) {
    uint64_t result = 0;
    for (size_t i = 0; i < bitmap->num_chunks; i++) {
        result |= bitmap->chunks[i];
    }
    return result == 0;
}

void bitmap_destroy(struct bitmap *bitmap) {
    free(bitmap->chunks);
    bitmap->chunks = NULL;
    bitmap->num_chunks = 0;
}
