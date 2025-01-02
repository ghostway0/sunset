#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

typedef uint64_t Limb;

#define LIMB_SIZE_BITS sizeof(Limb) * CHAR_BIT

struct Bitmask {
    size_t num_chunks;
    // chunks are ordered from the least significant limb to the most
    // significant
    uint64_t *chunks;
} typedef Bitmask;

void bitmask_init(size_t size, Bitmask *bitmask_out);

void bitmask_init_full(size_t size, Bitmask *bitmask_out);

void bitmask_init_empty(size_t size, Bitmask *bitmask_out);

void bitmask_destroy(Bitmask *bitmask);

bool bitmask_is_set(Bitmask const *bitmask, size_t index);

void bitmask_set(Bitmask *bitmask, size_t index);

size_t bitmask_ctz(Bitmask const *bitmask);

void bitmask_resize(Bitmask *bitmask, size_t new_size);

bool bitmask_is_eql(Bitmask const *bitmask, Bitmask const *other);

bool bitmask_is_superset(
        Bitmask const *bitmask, Bitmask const *other);

size_t bitmask_popcount(Bitmask const *bitmask);

void bitmask_lsb_reset(Bitmask *bitmask);

bool bitmask_is_zero(Bitmask const *bitmask);
