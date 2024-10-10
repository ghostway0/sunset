#pragma once

#include "vector.h"

#include <stddef.h>

int base64_encode(char const *in, size_t in_size, vector(char) * out);

int base64_decode(char const *in, size_t in_size, vector(char) * out);
