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
#include "sunset/io.h"
#include "sunset/mtl_file.h"
#include "sunset/obj_file.h"
#include "sunset/vector.h"

void parse_empty(void **state) {
    unused(state);

    uint8_t const str[] = "";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Material) mtls;
    vector_init(mtls);

    int err = mtl_file_parse(&reader, &mtls);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(mtls), 0);

    vector_destroy(mtls);
}

void single_material(void **state) {
    unused(state);

    uint8_t const str[] =
            "newmtl Material1\n"
            "Kd 0.8 0.0 0.2\n"
            "Ks 1.0 1.0 1.0\n"
            "Ns 100.0\n"
            "d 1.0\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Material) mtls;
    vector_init(mtls);

    int err = mtl_file_parse(&reader, &mtls);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(mtls), 1);
    assert_string_equal(mtls[0].name, "Material1");
    assert_float_equal(mtls[0].kd[0], 0.8f, EPSILON);
    assert_float_equal(mtls[0].kd[1], 0.0f, EPSILON);
    assert_float_equal(mtls[0].kd[2], 0.2f, EPSILON);
    assert_float_equal(mtls[0].ks[0], 1.0f, EPSILON);
    assert_float_equal(mtls[0].ns, 100.0f, EPSILON);
    assert_float_equal(mtls[0].d, 1.0f, EPSILON);
}

void multiple_materials(void **state) {
    unused(state);

    uint8_t const str[] =
            "newmtl Material1\n"
            "Kd 0.8 0.0 0.2\n"
            "newmtl Material2\n"
            "Kd 0.1 0.5 0.9\n";

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Material) mtls;
    vector_init(mtls);

    int err = mtl_file_parse(&reader, &mtls);
    assert_int_equal(err, 0);

    assert_int_equal(vector_size(mtls), 2);
    assert_string_equal(mtls[0].name, "Material1");
    assert_string_equal(mtls[1].name, "Material2");
}

void emission_map(void **state) {
    unused(state);

    todo();

    // uint8_t const str[] =
    //         "newmtl Material1\n"
    //         "map_Ke textures/emission.tga\n";
    //
    // ByteStream stream;
    // bstream_from_ro(str, sizeof(str) - 1, &stream);
    // Reader reader = {.ctx = &stream, .read = bstream_read};
    //
    // vector(Material) mtls;
    // vector_init(mtls);
    //
    // int err = mtl_file_parse(&reader, &mtls);
    // assert_int_equal(err, 0);
    //
    // assert_int_equal(vector_size(mtls), 1);
}

void invalid_format(void **state) {
    unused(state);

    uint8_t const str[] =
            "newmtl Material1\n"
            "Kd 0.8 0.0\n"; // missing a component in Kd

    ByteStream stream;
    bstream_from_ro(str, sizeof(str) - 1, &stream);
    Reader reader = {.ctx = &stream, .read = bstream_read};

    vector(Material) mtls;
    vector_init(mtls);

    int err = mtl_file_parse(&reader, &mtls);
    assert_int_equal(err, ERROR_INVALID_FORMAT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(parse_empty),
            cmocka_unit_test(single_material),
            cmocka_unit_test(multiple_materials),
            cmocka_unit_test(emission_map),
            cmocka_unit_test(invalid_format),
    };

    return cmocka_run_group_tests_name("mtl", tests, NULL, NULL);
}
