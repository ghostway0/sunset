#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct bitmap {
    size_t num_chunks;
    // chunks are ordered from the least significant limb to the most
    // significant
    uint64_t *chunks;
};

void bitmap_init(size_t size, struct bitmap *bitmap_out);

void bitmap_init_full(size_t size, struct bitmap *bitmap_out);

void bitmap_init_empty(size_t size, struct bitmap *bitmap_out);

void bitmap_destroy(struct bitmap *bitmap);

bool bitmap_is_set(struct bitmap const *bitmap, size_t index);

void bitmap_set(struct bitmap *bitmap, size_t index);

size_t bitmap_ctz(struct bitmap const *bitmap);

void bitmap_resize(struct bitmap *bitmap, size_t new_size);

bool bitmap_is_eql(struct bitmap const *bitmap, struct bitmap const *other);

bool bitmap_is_superset(
        struct bitmap const *bitmap, struct bitmap const *other);

size_t bitmap_popcount(struct bitmap const *bitmap);

void bitmap_lsb_reset(struct bitmap *bitmap);

bool bitmap_is_zero(struct bitmap const *bitmap);
