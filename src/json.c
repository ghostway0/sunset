#include <stdlib.h>
#include <string.h>

#include "sunset/errors.h"

#include "sunset/json.h"

#define MAX_JSON_SIZE 4096

struct parser {
    char const *buffer;
    size_t json_size;
    size_t cursor;
};

static int parse_value(struct parser *p, struct json_value *value_out);

static void skip_whitespace(struct parser *p) {
    while (p->cursor < p->json_size) {
        char c = p->buffer[p->cursor];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        p->cursor++;
    }
}

static int parse_string(struct parser *p, struct json_value *value_out) {
    if (p->buffer[p->cursor] != '"') {
        return -ERROR_PARSE;
    }

    p->cursor++;

    size_t start = p->cursor;

    for (;;) {
        if (p->buffer[p->cursor] == '"') {
            p->cursor++;
            break;
        }

        p->cursor++;
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
        p->cursor++;
    }

    while (p->cursor < p->json_size) {
        char c = p->buffer[p->cursor];

        if (c == '.' || c == 'e' || c == 'E') {
            p->cursor++;
            continue;
        }

        if (c < '0' || c > '9') {
            break;
        }

        p->cursor++;
    }

    value_out->type = JSON_NUMBER;
    value_out->data.number = strtod(p->buffer + start, NULL);

    return 0;
}

static int parse_object(struct parser *p, struct json_value *value_out) {
    if (p->buffer[p->cursor] != '{') {
        return -ERROR_PARSE;
    }

    vector_create(value_out->data.object, struct key_value);

    p->cursor++;

    for (;;) {
        skip_whitespace(p);

        if (p->buffer[p->cursor] == '}') {
            p->cursor++;
            break;
        }

        if (p->buffer[p->cursor] == ',') {
            p->cursor++;
            continue;
        }

        struct json_value key;
        int ret = parse_string(p, &key);

        if (ret) {
            return ret;
        }

        skip_whitespace(p);

        if (p->buffer[p->cursor] != ':') {
            return -ERROR_PARSE;
        }

        p->cursor++;

        struct json_value value;
        ret = parse_value(p, &value);

        if (ret) {
            return ret;
        }

        struct key_value kv = {
                .key = key.data.string,
                .value = malloc(sizeof(struct json_value)),
        };

        *kv.value = value;

        vector_append(value_out->data.object, kv);
    }

    value_out->type = JSON_OBJECT;

    return 0;
}

static int parse_array(struct parser *p, struct json_value *value_out) {
    if (p->buffer[p->cursor] != '[') {
        return -ERROR_PARSE;
    }

    p->cursor++;

    vector(struct json_value) array;
    vector_create(array, struct json_value);

    for (;;) {
        skip_whitespace(p);

        if (p->buffer[p->cursor] == ']') {
            p->cursor++;
            break;
        }

        if (p->buffer[p->cursor] == ',') {
            p->cursor++;
            continue;
        }

        struct json_value value;
        int ret = parse_value(p, &value);
        if (ret) {
            return ret;
        }

        vector_append(array, value);
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
        p->cursor += 4;
        return 0;
    }

    if (strncmp(p->buffer + p->cursor, "false", 5) == 0) {
        value_out->type = JSON_FALSE;
        p->cursor += 5;
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
