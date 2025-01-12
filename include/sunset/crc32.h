#pragma once

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_from_seed(uint32_t crc, uint8_t const *data, size_t size);

uint32_t crc32(uint8_t const *data, size_t size);
