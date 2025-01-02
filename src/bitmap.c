#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/bitmap.h"
#include "sunset/utils.h"

void bitmap_init(size_t size, Bitmap *bitmap_out) {
    bitmap_out->num_chunks = (size + LIMB_SIZE_BITS - 1) / LIMB_SIZE_BITS;
    bitmap_out->chunks = sunset_malloc(bitmap_out->num_chunks * sizeof(Limb));
}

void bitmap_init_full(size_t size, Bitmap *bitmap_out) {
    bitmap_init(size, bitmap_out);
    memset(bitmap_out->chunks,
            0xFFFFFFFF,
            bitmap_out->num_chunks * sizeof(Limb));
}

void bitmap_init_empty(size_t size, Bitmap *bitmap_out) {
    bitmap_init(size, bitmap_out);
    memset(bitmap_out->chunks, 0, bitmap_out->num_chunks * sizeof(Limb));
}

bool bitmap_is_set(Bitmap const *bitmap, size_t index) {
    return bitmap->chunks[index / LIMB_SIZE_BITS]
            & (1ULL << (index % LIMB_SIZE_BITS));
}

void bitmap_set(Bitmap *bitmap, size_t index) {
    bitmap->chunks[index / LIMB_SIZE_BITS] |=
            (1ULL << (index % LIMB_SIZE_BITS));
}

size_t bitmap_ctz(Bitmap const *bitmap) {
    for (size_t i = 0; i < bitmap->num_chunks; ++i) {
        Limb chunk = bitmap->chunks[i];

        if (chunk != 0) {
            return i * LIMB_SIZE_BITS + __builtin_ctzll(chunk);
        }
    }

    return bitmap->num_chunks * LIMB_SIZE_BITS;
}

void bitmap_resize(Bitmap *bitmap, size_t new_size) {
    bitmap->num_chunks = (new_size + LIMB_SIZE_BITS - 1) / LIMB_SIZE_BITS;
    bitmap->chunks = realloc(bitmap->chunks, bitmap->num_chunks * sizeof(Limb));
}

bool bitmap_is_eql(Bitmap const *bitmap, Bitmap const *other) {
    return memcmp(bitmap->chunks, other->chunks, bitmap->num_chunks * 8) == 0;
}

bool bitmap_is_superset(Bitmap const *bitmap, Bitmap const *other) {
    for (size_t i = 0; i < bitmap->num_chunks; ++i) {
        if ((bitmap->chunks[i] & other->chunks[i]) != other->chunks[i]) {
            return false;
        }
    }

    return true;
}

size_t bitmap_popcount(Bitmap const *bitmap) {
    size_t popcount = 0;

    for (size_t i = 0; i < bitmap->num_chunks; ++i) {
        popcount += __builtin_popcountll(bitmap->chunks[i]);
    }

    return popcount;
}

void bitmap_lsb_reset(Bitmap *bitmap) {
    for (size_t i = 0; i < bitmap->num_chunks; i--) {
        if (bitmap->chunks[i] != 0) {
            bitmap->chunks[i] &= bitmap->chunks[i] - 1;
            return;
        }
    }
}

// premature but vectorization is cool
bool bitmap_is_zero(Bitmap const *bitmap) {
    Limb result = 0;
    for (size_t i = 0; i < bitmap->num_chunks; i++) {
        result |= bitmap->chunks[i];
    }
    return result == 0;
}

void bitmap_destroy(Bitmap *bitmap) {
    free(bitmap->chunks);
    bitmap->chunks = NULL;
    bitmap->num_chunks = 0;
}
