#include <stddef.h>
#include <stdio.h>
#include <string.h>

// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>
// clang-format on

#include "sunset/base64.h"
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
    camera_init(
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
            },
            &camera);

    camera_rotate_absolute(&camera, GLM_PI / 2, 0.0f);

    vec3 direction = {1.0f, 0.0f, 0.0f};
    camera_vec_to_world(&camera, direction);

    camera_move(&camera, direction);

    assert_float_equal(camera.position[0], 0.0f, EPSILON);
    assert_float_equal(camera.position[1], 0.0f, EPSILON);
    assert_float_equal(camera.position[2], 100.0f, EPSILON);
}

void test_base64_encode(void **state) {
    unused(state);

    vector(char) encoded;
    vector_init(encoded, char);

    int err = base64_encode("Hello, World!", 13, &encoded);
    assert_int_equal(err, 0);

    assert_string_equal(encoded, "SGVsbG8sIFdvcmxkIQ==");

    vector_free(encoded);
}

void test_base64_decode(void **state) {
    unused(state);

    vector(char) decoded;
    vector_init(decoded, char);

    int err = base64_decode("SGVsbG8sIFdvcmxkIQ==", 20, &decoded);
    assert_int_equal(err, 0);

    assert_string_equal(decoded, "Hello, World!");

    vector_free(decoded);
}

void test_base64_invalid_input(void **state) {
    unused(state);

    vector(char) decoded;
    vector_init(decoded, char);

    int err = base64_decode("SGVsbG8sIFdvcmxkIQ=", 19, &decoded);
    assert_int_equal(err, -1);

    vector_free(decoded);
}

// bool should_split(struct quad_tree *tree, struct quad_node *node) {
//     unused(tree);
//
//     struct chunk *chunk = node->data;
//
//     return chunk->num_objects > 5;
// }
//
// void *split(struct quad_tree *tree, void *data, struct rect new_bounds) {
//     unused(tree);
//
//     struct chunk *chunk = data;
//
//     struct chunk *new_chunk = malloc(sizeof(struct chunk));
//     *new_chunk = *chunk;
//     new_chunk->bounds = new_bounds;
//
//     size_t in_new_bounds = 0;
//
//     for (size_t i = 0; i < chunk->num_objects; ++i) {
//         struct object *object = chunk->objects + i;
//
//         if (position_within_rect(object->position, new_bounds)) {
//             in_new_bounds++;
//             log_trace("object %zu: " vec3_format " in chunk " rect_format,
//                     i,
//                     vec3_args(object->position),
//                     rect_args(new_bounds));
//         }
//     }
//
//     log_trace("in_new_bounds: %zu", in_new_bounds);
//
//     new_chunk->num_objects = in_new_bounds;
//     new_chunk->objects = malloc(in_new_bounds * sizeof(struct object));
//
//     for (size_t i = 0, j = 0; i < chunk->num_objects; ++i) {
//         struct object *object = chunk->objects + i;
//
//         if (position_within_rect(object->position, new_bounds)) {
//             new_chunk->objects[j++] = *object;
//         }
//     }
//
//     chunk->num_objects -= in_new_bounds;
//
//     return new_chunk;
// }

// void test_octree(void **state) {
//     unused(state);
//
//     struct quad_tree tree;
//
//     struct rect root_bounds = {0, 0, 200, 200};
//     struct object *objects = calloc(10, sizeof(struct object));
//
//     // a few objects with different bounding boxes
//     struct object pobjects[10] = {
//             {.bounding_box = {{0, 0, 0}, {10, 10, 10}}},
//             ...
//     };
//
//     memcpy(objects, pobjects, sizeof(pobjects));
//
//     struct chunk root_chunk = {
//             .bounds = root_bounds,
//             .objects = objects,
//             .num_objects = 10,
//             .lights = NULL,
//             .num_lights = 0,
//     };
//
//     quad_tree_create(
//             3, 5, should_split, split, NULL, &root_chunk, root_bounds,
//             &tree);
//
//     for (size_t i = 0; i < 10; ++i) {
//         struct object *object = objects + i;
//         log_info("object %zu: " vec3_format, i, vec3_args(object->position));
//     }
//
//     quad_tree_destroy(&tree);
// }

int main(void) {
    const struct CMUnitTest general_tests[] = {
            cmocka_unit_test(test_ring_buffer),
            cmocka_unit_test(test_color_from_hex),
            cmocka_unit_test(test_camera_movement),
            cmocka_unit_test(test_base64_encode),
            cmocka_unit_test(test_base64_decode),
            cmocka_unit_test(test_base64_invalid_input),
    };

    return cmocka_run_group_tests(general_tests, NULL, NULL);
}
