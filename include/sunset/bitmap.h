#pragma once

#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t Limb;

#define LIMB_SIZE_BITS (sizeof(Limb) * CHAR_BIT)

struct Bitmap {
    // chunks are ordered from the least significant limb to the most
    // significant
    Limb *chunks;
    size_t num_chunks;
} typedef Bitmap;

void bitmap_init(size_t size, Bitmap *bitmap_out);

void bitmap_init_full(size_t size, Bitmap *bitmap_out);

void bitmap_init_empty(size_t size, Bitmap *bitmap_out);

void bitmap_destroy(Bitmap *bitmap);

bool bitmap_is_set(Bitmap const *bitmap, size_t index);

void bitmap_set(Bitmap *bitmap, size_t index);

size_t bitmap_ctz(Bitmap const *bitmap);

void bitmap_resize(Bitmap *bitmap, size_t new_size);

bool bitmap_is_eql(Bitmap const *bitmap, Bitmap const *other);

bool bitmap_is_superset(
        Bitmap const *bitmap, Bitmap const *other);

size_t bitmap_popcount(Bitmap const *bitmap);

void bitmap_lsb_reset(Bitmap *bitmap);

bool bitmap_is_zero(Bitmap const *bitmap);
