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
#include "sunset/byte_stream.h"
#include "sunset/camera.h"
#include "sunset/ecs.h"
#include "sunset/errors.h"
#include "sunset/images.h"
#include "sunset/io.h"
#include "sunset/mtl_file.h"
#include "sunset/obj_file.h"
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

    ring_buffer_destroy(&ring_buffer);
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
    camera_move_scaled(&camera, direction);

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

void test_obj_model_parse_empty(void **state) {
    unused(state);

    uint8_t const str[] = "";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model.vertices), 0);
    assert_int_equal(vector_size(model.normals), 0);
    assert_int_equal(vector_size(model.texcoords), 0);
    assert_int_equal(vector_size(model.faces), 0);
    assert_null(model.material_lib);
    assert_null(model.object_name);

    obj_model_destroy(&model);
}

void test_obj_model_parse_vertices(void **state) {
    unused(state);

    uint8_t const str[] =
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model.vertices), 2);
    assert_float_equal(model.vertices[0][0], 1.0f, EPSILON);

    obj_model_destroy(&model);
}

void test_obj_model_parse_normals(void **state) {
    unused(state);

    uint8_t const str[] =
            "vn 0.0 -1.0 0.0\n"
            "vn 0.0 1.0 0.0\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model.normals), 2);
    assert_float_equal(model.normals[0][1], -1.0f, EPSILON);

    obj_model_destroy(&model);
}

void test_obj_model_parse_texcoords(void **state) {
    unused(state);

    uint8_t const str[] =
            "vt 0.625 0.5\n"
            "vt 0.875 0.5\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model.texcoords), 2);
    assert_float_equal(model.texcoords[0][0], 0.625f, EPSILON);

    obj_model_destroy(&model);
}

void test_obj_model_parse_faces(void **state) {
    unused(state);

    uint8_t const str[] =
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n"
            "v -1.0 -1.0 1.0\n"
            "f 1/1/1 2/2/1 3/3/1\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model.faces), 1);
    assert_int_equal(vector_size(model.faces[0]), 3);
    assert_int_equal(model.faces[0][0].vertex_index, 0);

    obj_model_destroy(&model);
}

void test_obj_model_parse_material_lib(void **state) {
    unused(state);

    uint8_t const str[] = "mtllib my_material.mtl\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_string_equal(model.material_lib, "my_material.mtl");

    obj_model_destroy(&model);
}

void test_obj_model_parse_object_name(void **state) {
    unused(state);

    uint8_t const str[] = "o MyObject\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_string_equal(model.object_name, "MyObject");

    obj_model_destroy(&model);
}

void test_obj_model_parse_invalid_faces(void **state) {
    unused(state);
    uint8_t const str[] =
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n"
            "v -1.0 -1.0 1.0\n"
            "f 1/2 2//1 3\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, ERROR_INVALID_FORMAT);
}

void test_obj_model_parse_partial_faces(void **state) {
    unused(state);
    uint8_t const str[] =
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n"
            "v -1.0 -1.0 1.0\n"
            "f 1//2 2//1 3//3\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct obj_model model;
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model.faces), 1);

    assert_int_equal(model.faces[0][0].vertex_index, 0);
    assert_int_equal(model.faces[0][0].texcoord_index, 0xFFFFFFFF);
    assert_int_equal(model.faces[0][0].normal_index, 1);

    assert_int_equal(model.faces[0][1].vertex_index, 1);
    assert_int_equal(model.faces[0][1].texcoord_index, 0xFFFFFFFF);
    assert_int_equal(model.faces[0][1].normal_index, 0);

    assert_int_equal(model.faces[0][2].vertex_index, 2);
    assert_int_equal(model.faces[0][2].texcoord_index, 0xFFFFFFFF);
    assert_int_equal(model.faces[0][2].normal_index, 2);

    obj_model_destroy(&model);
}

void test_mtl_file_parse_empty(void **state) {
    unused(state);

    uint8_t const str[] = "";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct mtl_file mtl;
    int err = mtl_file_parse(&reader, &mtl);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(mtl.materials), 0);

    mtl_file_destroy(&mtl);
}

void test_mtl_file_parse_single_material(void **state) {
    unused(state);

    uint8_t const str[] =
            "newmtl Material1\n"
            "Kd 0.8 0.0 0.2\n"
            "Ks 1.0 1.0 1.0\n"
            "Ns 100.0\n"
            "d 1.0\n"
            "map_Kd textures/diffuse.png\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct mtl_file mtl;
    int err = mtl_file_parse(&reader, &mtl);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(mtl.materials), 1);
    assert_string_equal(mtl.materials[0].name, "Material1");
    assert_float_equal(mtl.materials[0].kd[0], 0.8f, EPSILON);
    assert_float_equal(mtl.materials[0].kd[1], 0.0f, EPSILON);
    assert_float_equal(mtl.materials[0].kd[2], 0.2f, EPSILON);
    assert_float_equal(mtl.materials[0].ks[0], 1.0f, EPSILON);
    assert_float_equal(mtl.materials[0].ns, 100.0f, EPSILON);
    assert_float_equal(mtl.materials[0].d, 1.0f, EPSILON);
    assert_string_equal(mtl.materials[0].map_kd, "textures/diffuse.png");

    mtl_file_destroy(&mtl);
}

void test_mtl_file_parse_multiple_materials(void **state) {
    unused(state);

    uint8_t const str[] =
            "newmtl Material1\n"
            "Kd 0.8 0.0 0.2\n"
            "newmtl Material2\n"
            "Kd 0.1 0.5 0.9\n"
            "map_Kd textures/diffuse2.jpg\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct mtl_file mtl;
    int err = mtl_file_parse(&reader, &mtl);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(mtl.materials), 2);
    assert_string_equal(mtl.materials[0].name, "Material1");
    assert_string_equal(mtl.materials[1].name, "Material2");
    assert_string_equal(mtl.materials[1].map_kd, "textures/diffuse2.jpg");

    mtl_file_destroy(&mtl);
}

void test_mtl_file_parse_emission_map(void **state) {
    unused(state);

    uint8_t const str[] =
            "newmtl Material1\n"
            "map_Ke textures/emission.tga\n";

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct mtl_file mtl;
    int err = mtl_file_parse(&reader, &mtl);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(mtl.materials), 1);
    assert_string_equal(mtl.materials[0].map_ke, "textures/emission.tga");

    mtl_file_destroy(&mtl);
}

void test_mtl_file_parse_invalid_format(void **state) {
    unused(state);

    uint8_t const str[] =
            "newmtl Material1\n"
            "Kd 0.8 0.0\n"; // missing a component in Kd

    struct byte_stream stream;
    byte_stream_from_data(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = byte_stream_read};

    struct mtl_file mtl;
    int err = mtl_file_parse(&reader, &mtl);
    assert_int_equal(err, ERROR_INVALID_FORMAT);
}

struct Position {
    float x, y;
} typedef Position;

struct Velocity {
    float x, y;
} typedef Velocity;

struct Health {
    int value;
} typedef Health;

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
    entity_builder_add_component(&builder, COMPONENT_ID(Velocity), &vel);
    entity_builder_add_component(&builder, COMPONENT_ID(Position), &pos);
    entity_builder_add_component(&builder, COMPONENT_ID(Health), &hea);
    entity_builder_finish(&builder);

    Position pos2 = {3.0f, 4.0f};
    Velocity vel2 = {0.1f, 0.3f};
    Health hea2 = {50};

    entity_builder_init(&builder, &ecs);
    entity_builder_add_component(&builder, COMPONENT_ID(Velocity), &vel2);
    entity_builder_add_component(&builder, COMPONENT_ID(Position), &pos2);
    entity_builder_add_component(&builder, COMPONENT_ID(Health), &hea2);
    entity_builder_finish(&builder);

    Position pos3 = {4.0f, 5.0f};
    Velocity vel3 = {0.0f, 0.0f};

    entity_builder_init(&builder, &ecs);
    entity_builder_add_component(&builder, COMPONENT_ID(Velocity), &vel3);
    entity_builder_add_component(&builder, COMPONENT_ID(Position), &pos3);
    entity_builder_finish(&builder);

    Bitmask system_mask;
    bitmask_init_empty(64, &system_mask);
    bitmask_set(&system_mask, COMPONENT_ID(Position));
    bitmask_set(&system_mask, COMPONENT_ID(Velocity));

    WorldIterator it = ecs_iterator_create(&ecs, system_mask);

    // we assume ordering for now
    {
        Position *p = (Position *)ecs_iterator_get_component(
                &it, COMPONENT_ID(Position));
        Velocity *v = (Velocity *)ecs_iterator_get_component(
                &it, COMPONENT_ID(Velocity));

        assert_float_equal(p->x, 1.0f, EPSILON);
        assert_float_equal(p->y, 2.0f, EPSILON);

        assert_float_equal(v->x, 0.1f, EPSILON);
        assert_float_equal(v->y, 0.2f, EPSILON);
    }

    ecs_iterator_advance(&it);

    {
        Position *p = (Position *)ecs_iterator_get_component(
                &it, COMPONENT_ID(Position));
        Velocity *v = (Velocity *)ecs_iterator_get_component(
                &it, COMPONENT_ID(Velocity));

        assert_float_equal(p->x, 3.0f, EPSILON);
        assert_float_equal(p->y, 4.0f, EPSILON);

        assert_float_equal(v->x, 0.1f, EPSILON);
        assert_float_equal(v->y, 0.3f, EPSILON);
    }

    ecs_iterator_advance(&it);

    {
        Position *p = (Position *)ecs_iterator_get_component(
                &it, COMPONENT_ID(Position));
        Velocity *v = (Velocity *)ecs_iterator_get_component(
                &it, COMPONENT_ID(Velocity));

        assert_float_equal(p->x, 4.0f, EPSILON);
        assert_float_equal(p->y, 5.0f, EPSILON);

        assert_float_equal(v->x, 0.0f, EPSILON);
        assert_float_equal(v->y, 0.0f, EPSILON);
    }

    ecs_iterator_destroy(&it);
}

int main(void) {
    const struct CMUnitTest general_tests[] = {
            cmocka_unit_test(test_ring_buffer),
            cmocka_unit_test(test_color_from_hex),
            cmocka_unit_test(test_camera_movement),
            cmocka_unit_test(test_base64_encode),
            cmocka_unit_test(test_base64_decode),
            cmocka_unit_test(test_base64_invalid_input),
            cmocka_unit_test(test_obj_model_parse_empty),
            cmocka_unit_test(test_obj_model_parse_vertices),
            cmocka_unit_test(test_obj_model_parse_normals),
            cmocka_unit_test(test_obj_model_parse_texcoords),
            cmocka_unit_test(test_obj_model_parse_faces),
            cmocka_unit_test(test_obj_model_parse_material_lib),
            cmocka_unit_test(test_obj_model_parse_object_name),
            cmocka_unit_test(test_obj_model_parse_invalid_faces),
            cmocka_unit_test(test_obj_model_parse_partial_faces),
            cmocka_unit_test(test_mtl_file_parse_empty),
            cmocka_unit_test(test_mtl_file_parse_single_material),
            cmocka_unit_test(test_mtl_file_parse_multiple_materials),
            cmocka_unit_test(test_mtl_file_parse_emission_map),
            cmocka_unit_test(test_mtl_file_parse_invalid_format),
            cmocka_unit_test(test_ecs),
    };

    return cmocka_run_group_tests(general_tests, NULL, NULL);
}
