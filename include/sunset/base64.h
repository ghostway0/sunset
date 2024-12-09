#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/vector.h"

int base64_encode(uint8_t const *in, size_t in_size, Vector(char) * out);

int base64_decode(char const *in, size_t in_size, Vector(uint8_t) * out);
