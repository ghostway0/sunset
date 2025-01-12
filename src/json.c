#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sunset/errors.h"
#include "sunset/vector.h"
#include "sunset/vfs.h"

#include "sunset/json.h"

#define bump(p)                                                            \
    ({                                                                     \
        if (p->cursor >= p->json_size) {                                   \
            return -ERROR_INVALID_FORMAT;                                  \
        }                                                                  \
        p->buffer[p->cursor++];                                            \
    })

#define multi_bump(p, n)                                                   \
    ({                                                                     \
        size_t start = p->cursor;                                          \
        p->cursor += n;                                                    \
        if (p->cursor >= p->json_size) {                                   \
            return -ERROR_INVALID_FORMAT;                                  \
        }                                                                  \
        p->buffer + start;                                                 \
    })

struct parser {
    char const *buffer;
    size_t json_size;
    size_t cursor;
    size_t line_num;
};

static int parse_value(struct parser *p, struct json_value *value_out);

static void skip_whitespace(struct parser *p) {
    while (p->cursor < p->json_size) {
        char c = p->buffer[p->cursor++];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            p->cursor--;
            break;
        }

        p->line_num += c == '\n';
    }
}

static int parse_string(struct parser *p, struct json_value *value_out) {
    if (bump(p) != '"') {
        return -ERROR_INVALID_FORMAT;
    }

    size_t start = p->cursor;

    bump(p);

    while (p->cursor < p->json_size && bump(p) != '"') {
    }

    value_out->type = JSON_STRING;
    value_out->data.string = sunset_malloc(p->cursor - start);

    memcpy((void *)value_out->data.string,
            p->buffer + start,
            p->cursor - start - 1);

    value_out->data.string[p->cursor - start - 1] = '\0';

    return 0;
}

static int parse_number(struct parser *p, struct json_value *value_out) {
    char const *start = p->buffer + p->cursor;
    char *end = NULL;

    double result = strtod(start, &end);

    if (start == end) {
        return -ERROR_INVALID_FORMAT;
    }

    p->cursor += (end - start);

    bool seen_special = false;
    for (char const *curr = start; curr < end; curr++) {
        seen_special |= *curr == '.' || *curr == 'e' || *curr == 'E';
    }

    if (seen_special) {
        value_out->type = JSON_NUMBER;
        value_out->data.number = result;
    } else {
        value_out->type = JSON_WHOLE_NUMBER;
        value_out->data.whole_number = (size_t)result;
    }

    return 0;
}

static int parse_object(struct parser *p, struct json_value *value_out) {
    int retval = 0;

    value_out->type = JSON_OBJECT;

    if (p->buffer[p->cursor] != '{') {
        return -ERROR_INVALID_FORMAT;
    }

    vector_init(value_out->data.object);

    bump(p);

    if (p->buffer[p->cursor] == '}') {
        bump(p);
        return 0;
    }

    for (;;) {
        skip_whitespace(p);

        struct json_value key;

        if ((retval = parse_string(p, &key))) {
            return retval;
        }

        skip_whitespace(p);

        if (p->buffer[p->cursor] != ':') {
            free(key.data.string);
            retval = -ERROR_INVALID_FORMAT;
            goto cleanup;
        }

        bump(p);

        struct json_value value;
        if ((retval = parse_value(p, &value))) {
            free(key.data.string);
            retval = -ERROR_INVALID_FORMAT;
            goto cleanup;
        }

        KeyValue kv = {
                .key = key.data.string,
                .value = value,
        };

        vector_append(value_out->data.object, kv);

        skip_whitespace(p);

        if (p->buffer[p->cursor] == '}') {
            bump(p);
            break;
        }

        if (p->buffer[p->cursor] != ',') {
            return -ERROR_INVALID_FORMAT;
        }

        bump(p);
    }

cleanup:
    if (retval != 0) {
        vector_destroy(value_out->data.object);
    }

    return retval;
}

static int parse_array(struct parser *p, struct json_value *value_out) {
    int retval;

    if (p->buffer[p->cursor] != '[') {
        return -ERROR_INVALID_FORMAT;
    }

    bump(p);

    vector(struct json_value) array;
    vector_init(array);

    for (;;) {
        skip_whitespace(p);

        struct json_value value;
        if ((retval = parse_value(p, &value))) {
            return retval;
        }

        vector_append(array, value);

        skip_whitespace(p);

        if (p->buffer[p->cursor] == ']') {
            bump(p);
            break;
        }

        if (p->buffer[p->cursor] != ',') {
            return -ERROR_INVALID_FORMAT;
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
        value_out->type = JSON_BOOLEAN;
        value_out->data.boolean = true;
        multi_bump(p, 4);
        return 0;
    }

    if (strncmp(p->buffer + p->cursor, "false", 5) == 0) {
        value_out->type = JSON_BOOLEAN;
        value_out->data.boolean = false;
        multi_bump(p, 5);
        return 0;
    }

    if (strncmp(p->buffer + p->cursor, "null", 4) == 0) {
        value_out->type = JSON_NULL;
        multi_bump(p, 4);
        return 0;
    }

    return -ERROR_INVALID_FORMAT;
}

int json_parse(char const *input,
        size_t input_size,
        struct json_value *value_out) {
    struct parser p = {
            .buffer = input,
            .json_size = input_size,
            .cursor = 0,
            .line_num = 1,
    };

    return parse_value(&p, value_out);
}

void json_value_destroy(struct json_value *json) {
    if (json->type == JSON_OBJECT) {
        for (size_t i = 0; i < vector_size(json->data.object); i++) {
            json_value_destroy(&json->data.object->value);
        }

        vector_destroy(json->data.object);
        return;
    }

    if (json->type == JSON_ARRAY) {
        for (size_t i = 0; i < vector_size(json->data.array); i++) {
            json_value_destroy(&json->data.array[i]);
        }

        vector_destroy(json->data.array);
        return;
    }

    if (json->type == JSON_STRING) {
        free(json->data.string);
    }
}

static void print_indent(VfsFile *file, size_t indent) {
    if (indent != (size_t)-1) {
        for (size_t i = 0; i < indent; i++) {
            vfs_file_print(file, " ");
        }
    }
}

void json_value_print(
        struct json_value *json, VfsFile *file, size_t indent) {
    switch (json->type) {
        case JSON_STRING:
            vfs_file_printf(file, "\"%s\"", json->data.string);
            break;
        case JSON_WHOLE_NUMBER:
            vfs_file_printf(file, "\"%zu\"", json->data.whole_number);
            break;
        case JSON_NUMBER:
            vfs_file_printf(file, "\"%f\"", json->data.number);
            break;
        case JSON_BOOLEAN:
            vfs_file_print(file, json->data.boolean ? "true" : "false");
            break;
        case JSON_NULL:
            vfs_file_print(file, "null");
            break;
        case JSON_OBJECT: {
            vfs_file_print(file, "{");
            for (size_t i = 0; i < vector_size(json->data.object); i++) {
                if (indent != (size_t)-1) {
                    vfs_file_print(file, "\n");
                    print_indent(file, indent + 2);
                }
                vfs_file_printf(file, "\"%s\": ", json->data.object[i].key);
                json_value_print(&json->data.object[i].value,
                        file,
                        indent != (size_t)-1 ? indent + 2 : indent);
                if (i != vector_size(json->data.object) - 1) {
                    vfs_file_print(file, indent != (size_t)-1 ? "," : ", ");
                }
            }
            if (indent != (size_t)-1) {
                vfs_file_print(file, "\n");
                print_indent(file, indent);
            }
            vfs_file_print(file, "}");
            break;
        }
        case JSON_ARRAY: {
            vfs_file_print(file, "[");
            for (size_t i = 0; i < vector_size(json->data.array); i++) {
                if (indent != (size_t)-1) {
                    vfs_file_print(file, "\n");
                    print_indent(file, indent + 2);
                }
                json_value_print(&json->data.array[i],
                        file,
                        indent != (size_t)-1 ? indent + 2 : indent);
                if (i != vector_size(json->data.array) - 1) {
                    vfs_file_print(file, indent != (size_t)-1 ? "," : ", ");
                }
            }
            if (indent != (size_t)-1) {
                vfs_file_print(file, "\n");
                print_indent(file, indent);
            }
            vfs_file_print(file, "]");
            break;
        }
        default:
            unreachable();
    }
}
