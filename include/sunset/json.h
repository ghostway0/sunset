#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "vector.h"

enum json_type {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_WHOLE_NUMBER,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL,
};

struct json_value {
    enum json_type type;
    union {
        char *string;
        double number;
        vector(struct key_value) object;
        vector(struct json_value) array;
        size_t whole_number;
        bool boolean;
    } data;
};

struct key_value {
    char const *key;
    struct json_value value;
};

int json_parse(
        char const *input, size_t input_size, struct json_value *value_out);

void json_value_free(struct json_value *json);

#define json_assert_type(value, T) \
    do { \
        if ((value)->type != (T)) { \
            return -ERROR_PARSE; \
        } \
    } while (0)
