#pragma once

#include "vector.h"

#include <stdint.h>
#include <stddef.h>

int base64_encode(uint8_t const *in, size_t in_size, vector(char) * out);

int base64_decode(char const *in, size_t in_size, vector(uint8_t) * out);
