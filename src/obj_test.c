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
#include "sunset/byte_stream.h"
#include "sunset/errors.h"
#include "sunset/io.h"
#include "sunset/obj_file.h"
#include "vector.h"

void parse_empty(void **state) {
    unused(state);

    uint8_t const str[] = "";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model), 0);
}

void parse_vertices(void **state) {
    unused(state);

    uint8_t const str[] =
            "o test\n"
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);

    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model), 1);

    assert_int_equal(vector_size(model[0].vertices), 2);
    assert_float_equal(model[0].vertices[0][0], 1.0f, EPSILON);
}

void parse_normals(void **state) {
    unused(state);

    uint8_t const str[] =
            "vn 0.0 -1.0 0.0\n"
            "vn 0.0 1.0 0.0\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model[0].normals), 2);
    assert_float_equal(model[0].normals[0][1], -1.0f, EPSILON);
}

void parse_texcoords(void **state) {
    unused(state);

    uint8_t const str[] =
            "vt 0.625 0.5\n"
            "vt 0.875 0.5\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);

    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);
    assert_int_equal(vector_size(model), 1);

    assert_int_equal(vector_size(model[0].texcoords), 2);
    assert_float_equal(model[0].texcoords[0][0], 0.625f, EPSILON);
}

void parse_faces(void **state) {
    unused(state);

    uint8_t const str[] =
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n"
            "v -1.0 -1.0 1.0\n"
            "f 1/1/1 2/2/1 3/3/1\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(model[0].faces), 1);
    assert_int_equal(vector_size(model[0].faces[0]), 3);
    assert_int_equal(model[0].faces[0][0].vertex_index, 0);
}

void parse_material_lib(void **state) {
    unused(state);

    uint8_t const str[] = "mtllib my_material.mtl\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_string_equal(model[0].material_lib, "my_material.mtl");
}

void parse_object_name(void **state) {
    unused(state);

    uint8_t const str[] = "o MyObject\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, 0);

    assert_string_equal(model[0].object_name, "MyObject");
}

void parse_invalid_faces(void **state) {
    unused(state);
    uint8_t const str[] =
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n"
            "v -1.0 -1.0 1.0\n"
            "f 1/2 2//1 3\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) model;
    vector_init(model);
    int err = obj_model_parse(&reader, &model);
    assert_int_equal(err, ERROR_INVALID_FORMAT);
}

void parse_partial_faces(void **state) {
    unused(state);

    uint8_t const str[] =
            "v 1.0 -1.0 -1.0\n"
            "v 1.0 -1.0 1.0\n"
            "v -1.0 -1.0 1.0\n"
            "f 1//2 2//1 3//3\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Model) models;
    vector_init(models);
    int err = obj_model_parse(&reader, &models);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(models[0].faces), 1);

    assert_int_equal(models[0].faces[0][0].vertex_index, 0);
    assert_int_equal(models[0].faces[0][0].texcoord_index, 0xFFFFFFFF);
    assert_int_equal(models[0].faces[0][0].normal_index, 1);

    assert_int_equal(models[0].faces[0][1].vertex_index, 1);
    assert_int_equal(models[0].faces[0][1].texcoord_index, 0xFFFFFFFF);
    assert_int_equal(models[0].faces[0][1].normal_index, 0);

    assert_int_equal(models[0].faces[0][2].vertex_index, 2);
    assert_int_equal(models[0].faces[0][2].texcoord_index, 0xFFFFFFFF);
    assert_int_equal(models[0].faces[0][2].normal_index, 2);
}

int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(parse_empty),
            cmocka_unit_test(parse_vertices),
            cmocka_unit_test(parse_normals),
            cmocka_unit_test(parse_texcoords),
            cmocka_unit_test(parse_faces),
            cmocka_unit_test(parse_material_lib),
            cmocka_unit_test(parse_object_name),
            cmocka_unit_test(parse_invalid_faces),
            cmocka_unit_test(parse_partial_faces),
    };

    return cmocka_run_group_tests_name("obj", tests, NULL, NULL);
}
