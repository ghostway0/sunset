#include <stddef.h>
#include <stdio.h>
#include <string.h>

// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>
// clang-format on

#include "internal/utils.h"
#include "sunset/base64.h"
#include "sunset/bitmask.h"
#include "sunset/camera.h"
#include "sunset/ecs.h"
#include "sunset/images.h"
#include "sunset/ring_buffer.h"
#include "sunset/vector.h"

struct element {
    int x;
    int y;
};

static void test_ring_buffer(void **state) {
    unused(state);

    RingBuffer ring_buffer;
    ring_buffer_init(&ring_buffer, 16, sizeof(struct element));

    struct element e = {1, 2};
    assert_int_equal(ring_buffer_append(&ring_buffer, &e), 0);

    struct element e_out;
    assert_int_equal(ring_buffer_pop(&ring_buffer, &e_out), 0);
    assert_int_equal(e.x, e_out.x);
    assert_int_equal(e.y, e_out.y);

    ring_buffer_destroy(&ring_buffer);
}

void test_color_from_hex(void **state) {
    unused(state);

    Color c = color_from_hex("#FF0000");
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

    Camera camera;
    camera_init(
            (CameraState){
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    0.0f,
                    0.0f,

            },
            (CameraOptions){
                    0.1f,
                    100.0f,
                    45.0f,
                    0.75f,
            },
            &camera);

    camera_rotate_absolute(&camera, GLM_PI / 2, 0.0f);

    vec3 direction = {1.0f, 0.0f, 0.0f};
    camera_move_scaled(&camera, direction, 1.0f);

    assert_float_equal(camera.position[0], 100.0f, EPSILON);
    assert_float_equal(camera.position[1], 0.0f, EPSILON);
    assert_float_equal(camera.position[2], 0.0f, EPSILON);
}

void test_base64_encode(void **state) {
    unused(state);

    vector(char) encoded;
    vector_init(encoded);

    char *str = "Hello, World!";

    int err = base64_encode((uint8_t *)str, 13, &encoded);
    assert_int_equal(err, 0);

    assert_string_equal(encoded, "SGVsbG8sIFdvcmxkIQ==");

    vector_destroy(encoded);
}

void test_base64_decode(void **state) {
    unused(state);

    vector(uint8_t) decoded;
    vector_init(decoded);

    int err = base64_decode("SGVsbG8sIFdvcmxkIQ==", 20, &decoded);
    assert_int_equal(err, 0);

    assert_memory_equal(decoded, "Hello, World!", 13);

    vector_destroy(decoded);
}

void test_base64_invalid_input(void **state) {
    unused(state);

    vector(uint8_t) decoded;
    vector_init(decoded);

    int err = base64_decode("SGVsbG8sIFdvcmxkIQ=", 19, &decoded);
    assert_int_equal(err, -1);

    vector_destroy(decoded);
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
//             log_trace("object %zu: " vec3_format " in chunk "
//             rect_format,
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
//         log_info("object %zu: " vec3_format, i,
//         vec3_args(object->position));
//     }
//
//     quad_tree_destroy(&tree);
// }

typedef struct Position {
    float x, y;
} Position;

typedef struct Velocity {
    float x, y;
} Velocity;

typedef struct Health {
    int value;
} Health;

DECLARE_COMPONENT_ID(Position);
DECLARE_COMPONENT_ID(Velocity);
DECLARE_COMPONENT_ID(Health);

void test_ecs(void **state) {
    unused(state);

    World ecs;
    ecs_init(&ecs);

    REGISTER_COMPONENT(&ecs, Position);
    REGISTER_COMPONENT(&ecs, Velocity);
    REGISTER_COMPONENT(&ecs, Health);

    EntityBuilder builder;

    Position pos = {1.0f, 2.0f};
    Velocity vel = {0.1f, 0.2f};
    Health hea = {100};

    entity_builder_init(&builder, &ecs);
    entity_builder_add(&builder, COMPONENT_ID(Velocity), &vel);
    entity_builder_add(&builder, COMPONENT_ID(Position), &pos);
    entity_builder_add(&builder, COMPONENT_ID(Health), &hea);
    entity_builder_finish(&builder);

    Position pos2 = {3.0f, 4.0f};
    Velocity vel2 = {0.1f, 0.3f};
    Health hea2 = {50};

    entity_builder_init(&builder, &ecs);
    entity_builder_add(&builder, COMPONENT_ID(Velocity), &vel2);
    entity_builder_add(&builder, COMPONENT_ID(Position), &pos2);
    entity_builder_add(&builder, COMPONENT_ID(Health), &hea2);
    Index e2 = entity_builder_finish(&builder);

    Position pos3 = {4.0f, 5.0f};
    Velocity vel3 = {0.0f, 0.0f};

    entity_builder_init(&builder, &ecs);
    entity_builder_add(&builder, COMPONENT_ID(Velocity), &vel3);
    entity_builder_add(&builder, COMPONENT_ID(Position), &pos3);
    entity_builder_finish(&builder);

    Bitmask system_mask;
    bitmask_init_empty(64, &system_mask);
    bitmask_set(&system_mask, COMPONENT_ID(Position));
    bitmask_set(&system_mask, COMPONENT_ID(Velocity));

    WorldIterator it = worldit_create(&ecs, system_mask);

    // we assume ordering for now
    {
        Position *p = (Position *)worldit_get_component(
                &it, COMPONENT_ID(Position));
        Velocity *v = (Velocity *)worldit_get_component(
                &it, COMPONENT_ID(Velocity));

        assert_float_equal(p->x, 1.0f, EPSILON);
        assert_float_equal(p->y, 2.0f, EPSILON);

        assert_float_equal(v->x, 0.1f, EPSILON);
        assert_float_equal(v->y, 0.2f, EPSILON);
    }

    worldit_advance(&it);

    {
        Position *p = (Position *)worldit_get_component(
                &it, COMPONENT_ID(Position));
        Velocity *v = (Velocity *)worldit_get_component(
                &it, COMPONENT_ID(Velocity));

        assert_float_equal(p->x, 3.0f, EPSILON);
        assert_float_equal(p->y, 4.0f, EPSILON);

        assert_float_equal(v->x, 0.1f, EPSILON);
        assert_float_equal(v->y, 0.3f, EPSILON);
    }

    worldit_advance(&it);

    {
        Position *p = (Position *)worldit_get_component(
                &it, COMPONENT_ID(Position));
        Velocity *v = (Velocity *)worldit_get_component(
                &it, COMPONENT_ID(Velocity));

        assert_float_equal(p->x, 4.0f, EPSILON);
        assert_float_equal(p->y, 5.0f, EPSILON);

        assert_float_equal(v->x, 0.0f, EPSILON);
        assert_float_equal(v->y, 0.0f, EPSILON);
    }

    worldit_destroy(&it);

    ecs_remove_entity(&ecs, e2);

    Position pos4 = {5.0f, 6.0f};
    Velocity vel4 = {1.0f, 1.0f};
    Health hea4 = {1.0f};

    entity_builder_init(&builder, &ecs);
    entity_builder_add(&builder, COMPONENT_ID(Velocity), &vel4);
    entity_builder_add(&builder, COMPONENT_ID(Position), &pos4);
    entity_builder_add(&builder, COMPONENT_ID(Health), &hea4);
    Index e4 = entity_builder_finish(&builder);

    {
        Velocity *v = ecs_get_component(&ecs, e4, COMPONENT_ID(Velocity));
        assert_float_equal(v->x, vel4.x, EPSILON);
        assert_float_equal(v->x, vel4.y, EPSILON);
    }
}

int main(void) {
    const struct CMUnitTest general_tests[] = {
            cmocka_unit_test(test_ring_buffer),
            cmocka_unit_test(test_color_from_hex),
            cmocka_unit_test(test_camera_movement),
            cmocka_unit_test(test_base64_encode),
            cmocka_unit_test(test_base64_decode),
            cmocka_unit_test(test_base64_invalid_input),
            cmocka_unit_test(test_ecs),
    };

    return cmocka_run_group_tests(general_tests, NULL, NULL);
}
