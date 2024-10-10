#include "sunset/base64.h"
#include "sunset/vector.h"

#include <stddef.h>
#include <stdint.h>

static char const base64_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_encode(char const *in, size_t in_size, vector(char) * out) {
    size_t out_size = (in_size + 2) / 3 * 4;
    vector_reserve(*out, out_size);

    for (size_t i = 0; i < in_size; i += 3) {
        uint32_t value = (uint8_t)in[i] << 16
                | (i + 1 < in_size ? (uint8_t)in[i + 1] << 8 : 0)
                | (i + 2 < in_size ? (uint8_t)in[i + 2] : 0);

        vector_append(*out, base64_table[(value >> 18) & 0x3F]);
        vector_append(*out, base64_table[(value >> 12) & 0x3F]);
        vector_append(*out,
                i + 1 < in_size ? base64_table[(value >> 6) & 0x3F] : '=');
        vector_append(*out, i + 2 < in_size ? base64_table[value & 0x3F] : '=');
    }

    vector_append(*out, '\0');

    return 0;
}

int base64_decode(char const *in, size_t in_size, vector(char) * out) {
    if (in_size % 4 != 0)
        return -1;

    size_t out_size = (in_size / 4) * 3;
    if (in[in_size - 1] == '=')
        out_size--;
    if (in[in_size - 2] == '=')
        out_size--;

    vector_reserve(*out, out_size);

    for (size_t i = 0; i < in_size; i += 4) {
        uint32_t value = 0;

        for (size_t j = 0; j < 4; j++) {
            char c = in[i + j];
            value <<= 6;
            value |= (c >= 'A' && c <= 'Z')  ? (c - 'A')
                    : (c >= 'a' && c <= 'z') ? (c - 'a' + 26)
                    : (c >= '0' && c <= '9') ? (c - '0' + 52)
                    : (c == '+')             ? 62
                    : (c == '/')             ? 63
                                             : 0;
        }

        vector_append(*out, (value >> 16) & 0xFF);
        if (in[i + 2] != '=')
            vector_append(*out, (value >> 8) & 0xFF);
        if (in[i + 3] != '=')
            vector_append(*out, value & 0xFF);
    }

    vector_append(*out, '\0');

    return 0;
}
