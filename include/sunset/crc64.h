#pragma once

#include <stddef.h>
#include <stdint.h>

uint64_t crc64(uint8_t const *data, size_t data_size);

uint64_t crc64_from_seed(
        uint64_t crc, uint8_t const *data, size_t data_size);
