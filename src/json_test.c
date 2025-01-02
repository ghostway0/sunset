#include <stddef.h>

// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>
#include <unistd.h>
// clang-format on

#include "sunset/errors.h"
#include "sunset/json.h"
#include "sunset/utils.h"
#include "sunset/vfs.h"

void test_json_parse_simple_object(void **state) {
    unused(state);

    char const *json = "{\"key\": \"value\"}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(vector_size(value.data.object), 1);
    assert_string_equal(value.data.object[0].key, "key");
    assert_int_equal(value.data.object[0].value.type, JSON_STRING);
    assert_string_equal(value.data.object[0].value.data.string, "value");

    json_value_destroy(&value);
}

void test_json_parse_number(void **state) {
    unused(state);

    char const *json = "{\"key\": 42.5}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_OBJECT);
    assert_float_equal(value.data.object[0].value.data.number, 42.5, EPSILON);

    json_value_destroy(&value);
}

void test_json_parse_boolean(void **state) {
    unused(state);

    char const *json = "{\"key\": true}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(value.data.object[0].value.type, JSON_BOOLEAN);
    assert_true(value.data.object[0].value.data.boolean);

    json_value_destroy(&value);
}

void test_json_parse_array(void **state) {
    unused(state);

    char const *json = "{\"key\": [1, 2.0, true, \"value\"]}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);

    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(value.data.object[0].value.type, JSON_ARRAY);
    assert_int_equal(vector_size(value.data.object[0].value.data.array), 4);

    assert_int_equal(
            value.data.object[0].value.data.array[0].type, JSON_WHOLE_NUMBER);
    assert_int_equal(
            value.data.object[0].value.data.array[0].data.whole_number, 1);

    assert_int_equal(
            value.data.object[0].value.data.array[1].type, JSON_NUMBER);
    assert_float_equal(
            value.data.object[0].value.data.array[1].data.number, 2.0, EPSILON);
    assert_int_equal(
            value.data.object[0].value.data.array[2].type, JSON_BOOLEAN);
    assert_true(value.data.object[0].value.data.array[2].data.boolean);

    assert_int_equal(
            value.data.object[0].value.data.array[3].type, JSON_STRING);
    assert_string_equal(
            value.data.object[0].value.data.array[3].data.string, "value");

    json_value_destroy(&value);
}

void test_json_parse_toplevel_array(void **state) {
    unused(state);

    char const *json = "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);
    assert_int_equal(value.type, JSON_ARRAY);
    assert_int_equal(vector_size(value.data.array), 11);

    json_value_destroy(&value);
}

void test_json_parse_unbalanced_braces(void **state) {
    unused(state);

    char const *json = "{\"key\": 42.5, \"key2\": 42.5";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_INVALID_FORMAT);
    json_value_destroy(&value);
}

void test_json_parse_unterminated_string(void **state) {
    unused(state);

    char const *json = "{\"key\": \"value}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_INVALID_FORMAT);

    json_value_destroy(&value);
}

void test_json_parse_missing_comma(void **state) {
    unused(state);

    char const *json = "{\"key1\": \"value1\" \"key2\": \"value2\"}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_INVALID_FORMAT);

    json_value_destroy(&value);
}

void test_json_parse_extra_comma(void **state) {
    unused(state);

    char const *json = "{\"key1\": \"value1\",}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_INVALID_FORMAT);

    json_value_destroy(&value);
}

void test_json_parse_null_key(void **state) {
    unused(state);

    char const *json = "{null: \"value\"}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_INVALID_FORMAT);

    json_value_destroy(&value);
}

void test_json_parse_invalid_number_format(void **state) {
    unused(state);

    char const *json = "{\"key\": 42.5.6}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, -ERROR_INVALID_FORMAT);

    json_value_destroy(&value);
}

void test_json_parse_weird_number(void **state) {
    unused(state);

    char const *json = "-8.742278e-008";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);

    assert_int_equal(value.type, JSON_NUMBER);
    assert_float_equal(value.data.number, -8.742278e-008, EPSILON);
}

void test_json_parse_empty_object(void **state) {
    unused(state);

    char const *json = "{}";

    struct json_value value;
    int err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);

    assert_int_equal(value.type, JSON_OBJECT);
    assert_int_equal(vector_size(value.data.object), 0);
}

void test_json_value_print(void **state) {
    unused(state);

    int err;

    char const *json = "{\"key\": {\"key2\": \"value2\"}}";

    char test_buffer[strlen(json) + 1];
    memset(test_buffer, 0, sizeof(test_buffer));

    struct json_value value;
    err = json_parse(json, strlen(json), &value);
    assert_int_equal(err, 0);

    VfsFile temp_file;
    err = vfs_create_tempfile(&temp_file);
    assert_int_equal(err, 0);

    json_value_print(&value, &temp_file, -1);
    vfs_file_seek(&temp_file, VFS_SEEK_SET, 0);

    assert_int_equal(vfs_file_size(&temp_file), strlen(json));
    assert_int_equal(
            vfs_file_read(&temp_file, strlen(json), test_buffer), strlen(json));

    assert_string_equal(test_buffer, "{\"key\": {\"key2\": \"value2\"}}");

    vfs_close(&temp_file);
    json_value_destroy(&value);
}

int main(void) {
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
            cmocka_unit_test(test_json_parse_weird_number),
            cmocka_unit_test(test_json_parse_empty_object),
            cmocka_unit_test(test_json_value_print),
    };

    return cmocka_run_group_tests(json_tests, NULL, NULL);
}
