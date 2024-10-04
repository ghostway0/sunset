#include <stddef.h>
#include <string.h>

// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>
// clang-format on

#include "sunset/camera.h"
#include "sunset/color.h"
#include "sunset/json.h"
#include "sunset/ring_buffer.h"
#include "sunset/utils.h"

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

    assert_float_equal(camera.position[0], 0.0f, 0.1f);
    assert_float_equal(camera.position[1], 0.0f, 0.1f);
    assert_float_equal(camera.position[2], 100.0f, 0.1f);
}

void test_json_parse(void **state) {
    unused(state);

    char const *json = "{\"key\": \"value\"}";

    struct json_value value;
    json_parse(json, strlen(json), &value);

    assert_int_equal(value.type, JSON_OBJECT);

    assert_int_equal(vector_size(value.data.object), 1);

    assert_string_equal(value.data.object[0].key, "key");

    assert_int_equal(value.data.object[0].value->type, JSON_STRING);
    assert_string_equal(value.data.object[0].value->data.string, "value");

    json = "{\"key\": 42}";

    json_parse(json, strlen(json), &value);

    assert_int_equal(value.type, JSON_OBJECT);
    assert_float_equal(value.data.object[0].value->data.number, 42.0, 0.1);

    json = "{\"key\": true}";

    json_parse(json, strlen(json), &value);

    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(value.data.object[0].value->type, JSON_TRUE);

    json = "{\"key\": [1, 2, true, \"value\"]}";

    json_parse(json, strlen(json), &value);

    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(value.data.object[0].value->type, JSON_ARRAY);
    assert_int_equal(vector_size(value.data.object[0].value->data.array), 4);

    assert_int_equal(value.data.object[0].value->data.array[0].type, JSON_NUMBER);
    assert_int_equal(value.data.object[0].value->data.array[0].data.number, 1);
    assert_int_equal(value.data.object[0].value->data.array[1].type, JSON_NUMBER);
    assert_int_equal(value.data.object[0].value->data.array[1].data.number, 2);
    assert_int_equal(value.data.object[0].value->data.array[2].type, JSON_TRUE);
    assert_int_equal(value.data.object[0].value->data.array[3].type, JSON_STRING);
    assert_string_equal(value.data.object[0].value->data.array[3].data.string, "value");
}

int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(test_ring_buffer),
            cmocka_unit_test(test_color_from_hex),
            cmocka_unit_test(test_camera_movement),
            cmocka_unit_test(test_json_parse),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
