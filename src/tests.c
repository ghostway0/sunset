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
                    0.75f,
            });

    camera_rotate_absolute(&camera, GLM_PI / 2, 0.0f);

    vec3 direction = {1.0f, 0.0f, 0.0f};
    camera_vec_to_world(&camera, direction);

    camera_move(&camera, direction);

    assert_float_equal(camera.position[0], 0.0f, EPSILON);
    assert_float_equal(camera.position[1], 0.0f, EPSILON);
    assert_float_equal(camera.position[2], 100.0f, EPSILON);
}

int main(void) {
    const struct CMUnitTest general_tests[] = {
            cmocka_unit_test(test_ring_buffer),
            cmocka_unit_test(test_color_from_hex),
            cmocka_unit_test(test_camera_movement),
    };

    return cmocka_run_group_tests(general_tests, NULL, NULL);
}
