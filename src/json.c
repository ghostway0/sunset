#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/errors.h"

#include "sunset/json.h"

#define MAX_JSON_SIZE 4096

#define bump(p)                                                                \
    ({                                                                         \
        if (p->cursor >= p->json_size) {                                       \
            return -ERROR_PARSE;                                               \
        }                                                                      \
        p->buffer[p->cursor++];                                                \
    })

#define multi_bump(p, n)                                                       \
    ({                                                                         \
        size_t start = p->cursor;                                              \
        p->cursor += n;                                                        \
        if (p->cursor >= p->json_size) {                                       \
            return -ERROR_PARSE;                                               \
        }                                                                      \
        p->buffer + start;                                                     \
    })

struct parser {
    char const *buffer;
    size_t json_size;
    size_t cursor;
};

static int parse_value(struct parser *p, struct json_value *value_out);

static int skip_whitespace(struct parser *p) {
    while (p->cursor < p->json_size) {
        char c = bump(p);
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            p->cursor--;
            break;
        }
    }

    return 0;
}

static int parse_string(struct parser *p, struct json_value *value_out) {
    if (bump(p) != '"') {
        return -ERROR_PARSE;
    }

    size_t start = p->cursor;

    bump(p);

    for (;;) {
        if (bump(p) == '"') {
            break;
        }
    }

    value_out->type = JSON_STRING;
    value_out->data.string = malloc(p->cursor - start);

    memcpy((void *)value_out->data.string,
            p->buffer + start,
            p->cursor - start - 1);

    ((char *)value_out->data.string)[p->cursor - start] = '\0';

    return 0;
}

static int parse_number(struct parser *p, struct json_value *value_out) {
    size_t start = p->cursor;

    if (p->buffer[p->cursor] == '-') {
        bump(p);
    }

    bool seen_special = false;

    while (p->cursor < p->json_size) {
        char c = bump(p);

        if (c == '.' || c == 'e' || c == 'E') {
            if (!seen_special) {
                seen_special = true;
                continue;
            } else {
                return -ERROR_PARSE;
            }
        }

        if (c < '0' || c > '9') {
            p->cursor--;
            break;
        }
    }

    if (seen_special) {
        value_out->type = JSON_NUMBER;
        value_out->data.number = strtod(p->buffer + start, NULL);
        return 0;
    } else {
        value_out->type = JSON_WHOLE_NUMBER;
        value_out->data.whole_number = strtoul(p->buffer + start, NULL, 10);
    }

    return 0;
}

static int parse_object(struct parser *p, struct json_value *value_out) {
    int retval;

    if (p->buffer[p->cursor] != '{') {
        return -ERROR_PARSE;
    }

    vector_create(value_out->data.object, struct key_value);

    p->cursor++;

    for (;;) {
        if ((retval = skip_whitespace(p))) {
            return retval;
        }

        struct json_value key;

        if ((retval = parse_string(p, &key))) {
            return retval;
        }

        if ((retval = skip_whitespace(p))) {
            return retval;
        }

        if (p->buffer[p->cursor] != ':') {
            return -ERROR_PARSE;
        }

        bump(p);

        struct json_value value;
        if ((retval = parse_value(p, &value))) {
            return retval;
        }

        struct key_value kv = {
                .key = key.data.string,
                .value = malloc(sizeof(struct json_value)),
        };

        *kv.value = value;

        vector_append(value_out->data.object, kv);

        if ((retval = skip_whitespace(p))) {
            return retval;
        }

        if (p->buffer[p->cursor] == '}') {
            bump(p);
            break;
        }

        if (p->buffer[p->cursor] != ',') {
            return -ERROR_PARSE;
        }

        bump(p);
    }

    value_out->type = JSON_OBJECT;

    return 0;
}

static int parse_array(struct parser *p, struct json_value *value_out) {
    int retval;

    if (p->buffer[p->cursor] != '[') {
        return -ERROR_PARSE;
    }

    bump(p);

    vector(struct json_value) array;
    vector_create(array, struct json_value);

    for (;;) {
        if ((retval = skip_whitespace(p))) {
            return retval;
        }

        struct json_value value;
        if ((retval = parse_value(p, &value))) {
            return retval;
        }

        vector_append(array, value);

        if ((retval = skip_whitespace(p))) {
            return retval;
        }

        if (p->buffer[p->cursor] == ']') {
            bump(p);
            break;
        }

        if (p->buffer[p->cursor] != ',') {
            return -ERROR_PARSE;
        }

        bump(p);
    }

    value_out->type = JSON_ARRAY;
    value_out->data.array = array;

    return 0;
}

static int parse_value(struct parser *p, struct json_value *value_out) {
    skip_whitespace(p);

    char c = p->buffer[p->cursor];

    if (c == '{') {
        return parse_object(p, value_out);
    }

    if (c == '[') {
        return parse_array(p, value_out);
    }

    if (c == '"') {
        return parse_string(p, value_out);
    }

    if ((c >= '0' && c <= '9') || c == '-') {
        return parse_number(p, value_out);
    }

    if (strncmp(p->buffer + p->cursor, "true", 4) == 0) {
        value_out->type = JSON_TRUE;
        multi_bump(p, 4);
        return 0;
    }

    if (strncmp(p->buffer + p->cursor, "false", 5) == 0) {
        value_out->type = JSON_FALSE;
        multi_bump(p, 5);
        return 0;
    }

    if (strncmp(p->buffer + p->cursor, "null", 4) == 0) {
        value_out->type = JSON_NULL;
        multi_bump(p, 4);
        return 0;
    }

    return -ERROR_PARSE;
}

int json_parse(
        char const *input, size_t input_size, struct json_value *value_out) {
    struct parser p = {
            .buffer = input,
            .json_size = input_size,
            .cursor = 0,
    };

    return parse_value(&p, value_out);
}
