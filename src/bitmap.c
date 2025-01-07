#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/bitmask.h"
#include "internal/utils.h"

void bitmask_init(size_t size, Bitmask *bitmask_out) {
    bitmask_out->num_chunks = (size + LIMB_SIZE_BITS - 1) / LIMB_SIZE_BITS;
    bitmask_out->chunks =
            sunset_malloc(bitmask_out->num_chunks * sizeof(Limb));
}

void bitmask_init_full(size_t size, Bitmask *bitmask_out) {
    bitmask_init(size, bitmask_out);
    memset(bitmask_out->chunks,
            0xFFFFFFFF,
            bitmask_out->num_chunks * sizeof(Limb));
}

void bitmask_init_empty(size_t size, Bitmask *bitmask_out) {
    bitmask_init(size, bitmask_out);
    memset(bitmask_out->chunks, 0, bitmask_out->num_chunks * sizeof(Limb));
}

bool bitmask_is_set(Bitmask const *bitmask, size_t index) {
    return bitmask->chunks[index / LIMB_SIZE_BITS]
            & (1ULL << (index % LIMB_SIZE_BITS));
}

void bitmask_set(Bitmask *bitmask, size_t index) {
    bitmask->chunks[index / LIMB_SIZE_BITS] |=
            (1ULL << (index % LIMB_SIZE_BITS));
}

size_t bitmask_ctz(Bitmask const *bitmask) {
    for (size_t i = 0; i < bitmask->num_chunks; ++i) {
        Limb chunk = bitmask->chunks[i];

        if (chunk != 0) {
            return i * LIMB_SIZE_BITS + __builtin_ctzll(chunk);
        }
    }

    return bitmask->num_chunks * LIMB_SIZE_BITS;
}

void bitmask_resize(Bitmask *bitmask, size_t new_size) {
    size_t new_chunk_count =
            (new_size + LIMB_SIZE_BITS - 1) / LIMB_SIZE_BITS;

    if (new_chunk_count != bitmask->num_chunks) {
        bitmask->chunks = sunset_realloc(
                bitmask->chunks, bitmask->num_chunks * sizeof(Limb));
    }
}

bool bitmask_is_eql(Bitmask const *bitmask, Bitmask const *other) {
    return memcmp(bitmask->chunks, other->chunks, bitmask->num_chunks * 8)
            == 0;
}

bool bitmask_is_superset(Bitmask const *bitmask, Bitmask const *other) {
    for (size_t i = 0; i < bitmask->num_chunks; ++i) {
        if ((bitmask->chunks[i] & other->chunks[i]) != other->chunks[i]) {
            return false;
        }
    }

    return true;
}

size_t bitmask_popcount(Bitmask const *bitmask) {
    size_t popcount = 0;

    for (size_t i = 0; i < bitmask->num_chunks; ++i) {
        popcount += __builtin_popcountll(bitmask->chunks[i]);
    }

    return popcount;
}

void bitmask_lsb_reset(Bitmask *bitmask) {
    for (size_t i = 0; i < bitmask->num_chunks; i--) {
        if (bitmask->chunks[i] != 0) {
            bitmask->chunks[i] &= bitmask->chunks[i] - 1;
            return;
        }
    }
}

// premature but vectorization is cool
bool bitmask_is_zero(Bitmask const *bitmask) {
    Limb result = 0;
    for (size_t i = 0; i < bitmask->num_chunks; i++) {
        result |= bitmask->chunks[i];
    }
    return result == 0;
}

void bitmask_destroy(Bitmask *bitmask) {
    free(bitmask->chunks);
    bitmask->chunks = NULL;
    bitmask->num_chunks = 0;
}
