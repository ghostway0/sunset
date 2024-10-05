#include <assert.h>
#include <string.h>

#include <cglm/cglm.h>
#include <sys/mman.h>

#include "sunset/errors.h"
#include "sunset/gltf.h"
#include "sunset/vector.h"

#include "sunset/json.h"

static void gltf_file_init(struct gltf_file *file) {
    vector_create(file->accessors, struct accessor);
    vector_create(file->buffers, struct gltf_buffer);
    vector_create(file->buffer_views, struct gltf_buffer_view);
    vector_create(file->meshes, struct mesh);
    vector_create(file->nodes, struct node);
    vector_create(file->scenes, struct gltf_scene);
}

static int parse_accessor_json(
        const struct json_value *json, struct accessor *accessor_out) {
    if (json->type != JSON_OBJECT) {
        return -ERROR_PARSE;
    }

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (kv.value->type == JSON_NULL) {
            continue;
        }

        if (strcmp(kv.key, "bufferView") == 0) {
            json_assert_type(kv.value, JSON_WHOLE_NUMBER);
            accessor_out->buffer_view = kv.value->data.whole_number;
        } else if (strcmp(kv.key, "byteOffset") == 0) {
            json_assert_type(kv.value, JSON_WHOLE_NUMBER);
            accessor_out->byte_offset = kv.value->data.whole_number;
        } else if (strcmp(kv.key, "componentType") == 0) {
            json_assert_type(kv.value, JSON_WHOLE_NUMBER);
            accessor_out->component_type = kv.value->data.whole_number;
        } else if (strcmp(kv.key, "count") == 0) {
            json_assert_type(kv.value, JSON_WHOLE_NUMBER);
            accessor_out->count = kv.value->data.number;
        } else if (strcmp(kv.key, "type") == 0) {
            json_assert_type(kv.value, JSON_STRING);

            if (strcmp(kv.value->data.string, "SCALAR") == 0) {
                accessor_out->type = ACCESSOR_SCALAR;
            } else if (strcmp(kv.value->data.string, "VEC2") == 0) {
                accessor_out->type = ACCESSOR_VEC2;
            } else if (strcmp(kv.value->data.string, "VEC3") == 0) {
                accessor_out->type = ACCESSOR_VEC3;
            } else if (strcmp(kv.value->data.string, "VEC4") == 0) {
                accessor_out->type = ACCESSOR_VEC4;
            } else if (strcmp(kv.value->data.string, "MAT2") == 0) {
                accessor_out->type = ACCESSOR_MAT2;
            } else if (strcmp(kv.value->data.string, "MAT3") == 0) {
                accessor_out->type = ACCESSOR_MAT3;
            } else if (strcmp(kv.value->data.string, "MAT4") == 0) {
                accessor_out->type = ACCESSOR_MAT4;
            } else {
                return -ERROR_PARSE;
            }
        }
    }

    return 0;
}

static int parse_accessors_json(const struct json_value *json,
        vector(struct accessor) * accessors_out) {
    json_assert_type(json, JSON_ARRAY);

    for (size_t i = 0; i < vector_size(json->data.array); i++) {
        struct json_value *accessor_json = &json->data.array[i];

        struct accessor accessor = {};
        if (parse_accessor_json(accessor_json, &accessor)) {
            return -ERROR_PARSE;
        }

        vector_append(*accessors_out, accessor);
    }

    return 0;
}

static int parse_buffers_json(
        const struct json_value *json, vector(struct gltf_buffer) * buffers) {
    json_assert_type(json, JSON_ARRAY);

    for (size_t i = 0; i < vector_size(json->data.array); i++) {
        struct json_value *buffer_json = &json->data.array[i];

        json_assert_type(buffer_json, JSON_OBJECT);

        struct gltf_buffer buffer;

        for (size_t j = 0; j < vector_size(buffer_json->data.object); j++) {
            struct key_value kv = buffer_json->data.object[j];

            if (kv.value->type == JSON_NULL) {
                continue;
            }

            if (strcmp(kv.key, "uri") == 0) {
                json_assert_type(kv.value, JSON_STRING);
                buffer.uri = kv.value->data.string;
            } else if (strcmp(kv.key, "byteLength") == 0) {
                json_assert_type(kv.value, JSON_WHOLE_NUMBER);
                buffer.byte_length = kv.value->data.whole_number;
            }
        }

        vector_append(*buffers, buffer);
    }

    return 0;
}

static int parse_buffer_views_json(const struct json_value *json,
        vector(struct gltf_buffer_view) * buffer_views_out) {
    json_assert_type(json, JSON_ARRAY);

    for (size_t i = 0; i < vector_size(json->data.array); i++) {
        struct json_value *buffer_view_json = &json->data.array[i];

        json_assert_type(buffer_view_json, JSON_OBJECT);

        struct gltf_buffer_view buffer_view;

        for (size_t j = 0; j < vector_size(buffer_view_json->data.object);
                j++) {
            struct key_value kv = buffer_view_json->data.object[j];

            if (kv.value->type == JSON_NULL) {
                continue;
            }

            if (strcmp(kv.key, "buffer") == 0) {
                json_assert_type(kv.value, JSON_WHOLE_NUMBER);
                buffer_view.buffer = kv.value->data.whole_number;
            } else if (strcmp(kv.key, "byteLength") == 0) {
                json_assert_type(kv.value, JSON_WHOLE_NUMBER);
                buffer_view.byte_length = kv.value->data.whole_number;
            } else if (strcmp(kv.key, "byteOffset") == 0) {
                json_assert_type(kv.value, JSON_WHOLE_NUMBER);
                buffer_view.byte_offset = kv.value->data.whole_number;
            } else if (strcmp(kv.key, "byteStride") == 0) {
                json_assert_type(kv.value, JSON_WHOLE_NUMBER);
                buffer_view.byte_stride = kv.value->data.whole_number;
            } else if (strcmp(kv.key, "target") == 0) {
                json_assert_type(kv.value, JSON_WHOLE_NUMBER);
                buffer_view.target = kv.value->data.whole_number;
            }
        }

        vector_append(*buffer_views_out, buffer_view);
    }

    return 0;
}

static int parse_primitive(
        const struct json_value *json, struct gltf_primitive *primitive_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (kv.value->type == JSON_NULL) {
            continue;
        }

        if (strcmp(kv.key, "attributes") == 0) {
            memset(primitive_out->attributes,
                    0,
                    sizeof(primitive_out->attributes));

            json_assert_type(kv.value, JSON_OBJECT);

            for (size_t j = 0; j < vector_size(kv.value->data.object); j++) {
                struct key_value attribute_kv = kv.value->data.object[j];

                if (attribute_kv.value->type == JSON_NULL) {
                    continue;
                }

                json_assert_type(attribute_kv.value, JSON_WHOLE_NUMBER);

                if (strcmp(attribute_kv.key, "POSITION") == 0) {
                    json_assert_type(attribute_kv.value, JSON_WHOLE_NUMBER);
                    primitive_out->attributes[PRIMITIVE_POSITION] =
                            attribute_kv.value->data.whole_number;
                } else if (strcmp(attribute_kv.key, "NORMAL") == 0) {
                    json_assert_type(attribute_kv.value, JSON_WHOLE_NUMBER);
                    primitive_out->attributes[PRIMITIVE_NORMAL] =
                            attribute_kv.value->data.whole_number;
                } else if (strcmp(attribute_kv.key, "TANGENT") == 0) {
                    json_assert_type(attribute_kv.value, JSON_WHOLE_NUMBER);
                    primitive_out->attributes[PRIMITIVE_TANGENT] =
                            attribute_kv.value->data.whole_number;
                } else if (strcmp(attribute_kv.key, "TEXCOORD_0") == 0) {
                    json_assert_type(attribute_kv.value, JSON_WHOLE_NUMBER);
                    primitive_out->attributes[PRIMITIVE_TEXCOORD_0] =
                            attribute_kv.value->data.whole_number;
                } else if (strcmp(attribute_kv.key, "TEXCOORD_1") == 0) {
                    json_assert_type(attribute_kv.value, JSON_WHOLE_NUMBER);
                    primitive_out->attributes[PRIMITIVE_TEXCOORD_1] =
                            attribute_kv.value->data.whole_number;
                } else if (strcmp(attribute_kv.key, "COLOR_0") == 0) {
                    json_assert_type(attribute_kv.value, JSON_WHOLE_NUMBER);
                    primitive_out->attributes[PRIMITIVE_COLOR_0] =
                            attribute_kv.value->data.whole_number;
                } else if (strcmp(attribute_kv.key, "JOINTS_0") == 0) {
                    primitive_out->attributes[PRIMITIVE_JOINTS_0] =
                            attribute_kv.value->data.whole_number;
                }
            }
        } else if (strcmp(kv.key, "indices") == 0) {
            json_assert_type(kv.value, JSON_WHOLE_NUMBER);
            primitive_out->indices = kv.value->data.whole_number;
        } else if (strcmp(kv.key, "mode") == 0) {
            json_assert_type(kv.value, JSON_WHOLE_NUMBER);

            if (kv.value->data.whole_number >= NUM_PRIMITIVE_TYPES) {
                return -ERROR_PARSE;
            }

            primitive_out->mode = kv.value->data.whole_number;
        } else if (strcmp(kv.key, "material") == 0) {
            json_assert_type(kv.value, JSON_WHOLE_NUMBER);
            primitive_out->material = kv.value->data.whole_number;
        }
    }

    return 0;
}

static int parse_meshes_json(
        const struct json_value *json, vector(struct gltf_mesh) * mesh_out) {
    json_assert_type(json, JSON_ARRAY);

    for (size_t i = 0; i < vector_size(json->data.array); i++) {
        struct json_value *mesh_json = &json->data.array[i];
        json_assert_type(mesh_json, JSON_OBJECT);

        struct gltf_mesh mesh;
        vector_create(mesh.primitives, struct gltf_primitive);

        for (size_t j = 0; j < vector_size(mesh_json->data.object); j++) {
            struct key_value kv = mesh_json->data.object[j];

            if (kv.value->type == JSON_NULL) {
                continue;
            }

            if (strcmp(kv.key, "primitives") == 0) {
                json_assert_type(kv.value, JSON_ARRAY);

                for (size_t k = 0; k < vector_size(kv.value->data.array); k++) {
                    struct json_value *primitive_json =
                            &kv.value->data.array[k];

                    struct gltf_primitive primitive;
                    if (parse_primitive(primitive_json, &primitive)) {
                        return -ERROR_PARSE;
                    }

                    vector_append(mesh.primitives, primitive);
                }
            }
        }

        vector_append(*mesh_out, mesh);
    }

    return 0;
}

int parse_gltf_json(const struct json_value *json, struct gltf_file *file_out) {
    int retval = 0;

    if (json->type != JSON_OBJECT) {
        return -ERROR_PARSE;
    }

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "accessors") == 0) {
            if ((retval = parse_accessors_json(
                         kv.value, &file_out->accessors))) {
                return retval;
            }
        } else if (strcmp(kv.key, "buffers") == 0) {
            if ((retval = parse_buffers_json(kv.value, &file_out->buffers))) {
                return retval;
            }
        } else if (strcmp(kv.key, "bufferViews") == 0) {
            if ((retval = parse_buffer_views_json(
                         kv.value, &file_out->buffer_views))) {
                return retval;
            }
        } else if (strcmp(kv.key, "meshes") == 0) {
            if ((retval = parse_meshes_json(kv.value, &file_out->meshes))) {
                return retval;
            }
        } else if (strcmp(kv.key, "nodes") == 0) {
            // TODO
        } else if (strcmp(kv.key, "scenes") == 0) {
            // TODO
        }
    }

    return 0;
}

int gltf_parse(FILE *file, struct gltf_file *file_out) {
    int retval = 0;

    gltf_file_init(file_out);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);

    char *buffer =
            mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);

    struct json_value root;
    if ((retval = json_parse(buffer, file_size, &root))) {
        return retval;
    }

    return parse_gltf_json(&root, file_out);
}