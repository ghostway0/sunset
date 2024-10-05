#include <stddef.h>
#include <stdio.h>
#include <string.h>

// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>
// clang-format on

#include "sunset/camera.h"
#include "sunset/color.h"
#include "sunset/errors.h"
#include "sunset/json.h"
#include "sunset/ring_buffer.h"
#include "sunset/utils.h"

#define EPSILON 0.001

struct element {
    int x;
    int y;
};

static void test_ring_buffer(void **state) {
    unused(state);

    struct ring_buffer ring_buffer;
    ring_buffer_init(&ring_buffer, 16, sizeof(struct element));

    struct element e = {1, 2};
    assert_int_equal(ring_buffer_append(&ring_buffer, &e), 0);

    struct element e_out;
    assert_int_equal(ring_buffer_pop(&ring_buffer, &e_out), 0);
    assert_int_equal(e.x, e_out.x);
    assert_int_equal(e.y, e_out.y);

    ring_buffer_free(&ring_buffer);
}

void test_color_from_hex(void **state) {
    unused(state);

    struct color c = color_from_hex("#FF0000");
    assert_int_equal(c.r, 255);
    assert_int_equal(c.g, 0);
    assert_int_equal(c.b, 0);
    assert_int_equal(c.a, 255);

    c = color_from_hex("#5800FF");
    assert_int_equal(c.r, 0x58);
    assert_int_equal(c.g, 0);
    assert_int_equal(c.b, 255);
}

void test_camera_movement(void **state) {
    unused(state);

    struct camera camera;
    camera_init(&camera,
            (struct camera_state){
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    0.0f,
                    0.0f,

            },
            (struct camera_options){
                    0.1f,
                    100.0f,
                    45.0f,
            });

    camera_rotate_absolute(&camera, M_PI / 2, 0.0f);

    vec3 direction = {1.0f, 0.0f, 0.0f};
    camera_vec_to_world(&camera, direction);

    camera_move(&camera, direction);

    assert_float_equal(camera.position[0], 0.0f, EPSILON);
    assert_float_equal(camera.position[1], 0.0f, EPSILON);
    assert_float_equal(camera.position[2], 100.0f, EPSILON);
}

void test_json_parse_simple_object(void **state) {
    unused(state);

    char const *json = "{\"key\": \"value\"}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(vector_size(value.data.object), 1);
    assert_string_equal(value.data.object[0].key, "key");
    assert_int_equal(value.data.object[0].value->type, JSON_STRING);
    assert_string_equal(value.data.object[0].value->data.string, "value");
}

void test_json_parse_number(void **state) {
    unused(state);

    char const *json = "{\"key\": 42.5}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_OBJECT);
    assert_float_equal(value.data.object[0].value->data.number, 42.5, EPSILON);
}

void test_json_parse_boolean(void **state) {
    unused(state);

    char const *json = "{\"key\": true}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(value.data.object[0].value->type, JSON_TRUE);
}

void test_json_parse_array(void **state) {
    unused(state);

    char const *json = "{\"key\": [1, 2.0, true, \"value\"]}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);

    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(value.data.object[0].value->type, JSON_ARRAY);
    assert_int_equal(vector_size(value.data.object[0].value->data.array), 4);

    assert_int_equal(
            value.data.object[0].value->data.array[0].type, JSON_WHOLE_NUMBER);
    assert_int_equal(value.data.object[0].value->data.array[0].data.whole_number, 1);

    assert_int_equal(
            value.data.object[0].value->data.array[1].type, JSON_NUMBER);
    assert_float_equal(value.data.object[0].value->data.array[1].data.number, 2.0, EPSILON);
    assert_int_equal(value.data.object[0].value->data.array[2].type, JSON_TRUE);

    assert_int_equal(
            value.data.object[0].value->data.array[3].type, JSON_STRING);
    assert_string_equal(
            value.data.object[0].value->data.array[3].data.string, "value");
}

void test_json_parse_toplevel_array(void **state) {
    unused(state);

    char const *json = "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_ARRAY);
    assert_int_equal(vector_size(value.data.array), 11);
}

void test_json_parse_unbalanced_braces(void **state) {
    unused(state);

    char const *json = "{\"key\": 42.5, \"key2\": 42.5";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_PARSE);
}

void test_json_parse_unterminated_string(void **state) {
    unused(state);

    char const *json = "{\"key\": \"value}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_PARSE);
}

void test_json_parse_missing_comma(void **state) {
    unused(state);

    char const *json = "{\"key1\": \"value1\" \"key2\": \"value2\"}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_PARSE);
}

void test_json_parse_extra_comma(void **state) {
    unused(state);

    char const *json = "{\"key1\": \"value1\",}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_PARSE);
}

void test_json_parse_null_key(void **state) {
    unused(state);

    char const *json = "{null: \"value\"}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_PARSE);
}

void test_json_parse_invalid_number_format(void **state) {
    unused(state);

    char const *json = "{\"key\": 42.5.6}";
    
    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_PARSE);
}

int main(void) {
    int retval;

    const struct CMUnitTest general_tests[] = {
            cmocka_unit_test(test_ring_buffer),
            cmocka_unit_test(test_color_from_hex),
            cmocka_unit_test(test_camera_movement),
    };

    const struct CMUnitTest json_tests[] = {
            cmocka_unit_test(test_json_parse_simple_object),
            cmocka_unit_test(test_json_parse_number),
            cmocka_unit_test(test_json_parse_boolean),
            cmocka_unit_test(test_json_parse_array),
            cmocka_unit_test(test_json_parse_toplevel_array),
            cmocka_unit_test(test_json_parse_unbalanced_braces),
            cmocka_unit_test(test_json_parse_unterminated_string),
            cmocka_unit_test(test_json_parse_missing_comma),
            cmocka_unit_test(test_json_parse_extra_comma),
            cmocka_unit_test(test_json_parse_null_key),
            cmocka_unit_test(test_json_parse_invalid_number_format),
    };

    retval = cmocka_run_group_tests(general_tests, NULL, NULL);

    if (retval != 0) {
        return retval;
    }

    retval = cmocka_run_group_tests(json_tests, NULL, NULL);

    return retval;
}
