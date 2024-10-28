#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <cglm/cglm.h>
#include <log.h>

#include "sunset/errors.h"
#include "sunset/gltf.h"
#include "sunset/vector.h"

#include "sunset/json.h"

#define parse_multiple_json(json, T, vec_out, parse_fn)                        \
    do {                                                                       \
        json_assert_type(json, JSON_ARRAY);                                    \
        for (size_t i = 0; i < vector_size((json)->data.array); i++) {         \
            struct json_value *value_json = &(json)->data.array[i];            \
            T value;                                                           \
            if (parse_fn(value_json, &value)) {                                \
                return -ERROR_INVALID_FORMAT;                                           \
            }                                                                  \
            vector_append(vec_out, value);                                     \
        }                                                                      \
    } while (0)

#define string_copy(dest, from)                                                \
    do {                                                                       \
        size_t len = strlen(from);                                             \
        dest = malloc(len);                                                    \
        memset(dest, 0, len);                                                  \
        memcpy(dest, from, len + 1);                                           \
    } while (0)

static void gltf_file_init(struct gltf_file *file) {
    vector_init(file->accessors, struct accessor);
    vector_init(file->buffers, struct gltf_buffer);
    vector_init(file->buffer_views, struct gltf_buffer_view);
    vector_init(file->meshes, struct gltf_mesh);
    vector_init(file->nodes, struct gltf_node);
    vector_init(file->scenes, struct gltf_scene);
    vector_init(file->animations, struct gltf_animation);
    vector_init(file->images, struct gltf_image);
    vector_init(file->textures, struct gltf_texture);
    vector_init(file->materials, struct gltf_material);
}

static int parse_accessor_json(
        const struct json_value *json, struct accessor *accessor_out) {
    if (json->type != JSON_OBJECT) {
        return -ERROR_INVALID_FORMAT;
    }

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (kv.value.type == JSON_NULL) {
            continue;
        }

        if (strcmp(kv.key, "bufferView") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            accessor_out->buffer_view = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "byteOffset") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            accessor_out->byte_offset = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "componentType") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            accessor_out->component_type = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "count") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            accessor_out->count = kv.value.data.number;
        } else if (strcmp(kv.key, "type") == 0) {
            json_assert_type(&kv.value, JSON_STRING);

            if (strcmp(kv.value.data.string, "SCALAR") == 0) {
                accessor_out->type = ACCESSOR_SCALAR;
            } else if (strcmp(kv.value.data.string, "VEC2") == 0) {
                accessor_out->type = ACCESSOR_VEC2;
            } else if (strcmp(kv.value.data.string, "VEC3") == 0) {
                accessor_out->type = ACCESSOR_VEC3;
            } else if (strcmp(kv.value.data.string, "VEC4") == 0) {
                accessor_out->type = ACCESSOR_VEC4;
            } else if (strcmp(kv.value.data.string, "MAT2") == 0) {
                accessor_out->type = ACCESSOR_MAT2;
            } else if (strcmp(kv.value.data.string, "MAT3") == 0) {
                accessor_out->type = ACCESSOR_MAT3;
            } else if (strcmp(kv.value.data.string, "MAT4") == 0) {
                accessor_out->type = ACCESSOR_MAT4;
            } else {
                return -ERROR_INVALID_FORMAT;
            }
        }
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

            if (kv.value.type == JSON_NULL) {
                continue;
            }

            if (strcmp(kv.key, "uri") == 0) {
                json_assert_type(&kv.value, JSON_STRING);
                string_copy(buffer.uri, kv.value.data.string);
            } else if (strcmp(kv.key, "byteLength") == 0) {
                json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
                buffer.byte_length = kv.value.data.whole_number;
            }
        }

        if (buffer.byte_length == 0) {
            return -ERROR_INVALID_FORMAT;
        }

        if (buffer.uri == NULL) {
            return -ERROR_INVALID_FORMAT;
        }

        vector_append(*buffers, buffer);
    }

    if (vector_size(*buffers) == 0) {
        return -ERROR_INVALID_FORMAT;
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

            if (kv.value.type == JSON_NULL) {
                continue;
            }

            if (strcmp(kv.key, "buffer") == 0) {
                json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
                buffer_view.buffer = kv.value.data.whole_number;
            } else if (strcmp(kv.key, "byteLength") == 0) {
                json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
                buffer_view.byte_length = kv.value.data.whole_number;
            } else if (strcmp(kv.key, "byteOffset") == 0) {
                json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
                buffer_view.byte_offset = kv.value.data.whole_number;
            } else if (strcmp(kv.key, "byteStride") == 0) {
                json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
                buffer_view.byte_stride = kv.value.data.whole_number;
            } else if (strcmp(kv.key, "target") == 0) {
                json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
                buffer_view.target = kv.value.data.whole_number;
            }
        }

        vector_append(*buffer_views_out, buffer_view);
    }

    return 0;
}

static int parse_primitive_attribute(const struct key_value *attribute_kv,
        struct gltf_primitive *primitive_out) {
    if (strcmp(attribute_kv->key, "POSITION") == 0) {
        primitive_out->attributes[PRIMITIVE_POSITION] =
                attribute_kv->value.data.whole_number;
    } else if (strcmp(attribute_kv->key, "NORMAL") == 0) {
        primitive_out->attributes[PRIMITIVE_NORMAL] =
                attribute_kv->value.data.whole_number;
    } else if (strcmp(attribute_kv->key, "TANGENT") == 0) {
        primitive_out->attributes[PRIMITIVE_TANGENT] =
                attribute_kv->value.data.whole_number;
    } else if (strcmp(attribute_kv->key, "TEXCOORD_0") == 0) {
        primitive_out->attributes[PRIMITIVE_TEXCOORD_0] =
                attribute_kv->value.data.whole_number;
    } else if (strcmp(attribute_kv->key, "TEXCOORD_1") == 0) {
        primitive_out->attributes[PRIMITIVE_TEXCOORD_1] =
                attribute_kv->value.data.whole_number;
    } else if (strcmp(attribute_kv->key, "COLOR_0") == 0) {
        primitive_out->attributes[PRIMITIVE_COLOR_0] =
                attribute_kv->value.data.whole_number;
    } else if (strcmp(attribute_kv->key, "JOINTS_0") == 0) {
        primitive_out->attributes[PRIMITIVE_JOINTS_0] =
                attribute_kv->value.data.whole_number;
    } else {
        return -ERROR_INVALID_FORMAT;
    }

    return 0;
}

static int parse_primitive_attributes(
        const struct json_value *json, struct gltf_primitive *primitive_out) {
    json_assert_type(json, JSON_OBJECT);

    memset(primitive_out->attributes, 0, sizeof(primitive_out->attributes));

    for (size_t j = 0; j < vector_size(json->data.object); j++) {
        struct key_value attribute_kv = json->data.object[j];

        if (attribute_kv.value.type == JSON_NULL) {
            continue;
        }

        json_assert_type(&attribute_kv.value, JSON_WHOLE_NUMBER);
        if (parse_primitive_attribute(&attribute_kv, primitive_out) != 0) {
            return -ERROR_INVALID_FORMAT;
        }
    }

    return 0;
}

static int parse_primitive_properties(
        const struct key_value *kv, struct gltf_primitive *primitive_out) {
    if (strcmp(kv->key, "attributes") == 0) {
        return parse_primitive_attributes(&kv->value, primitive_out);
    } else if (strcmp(kv->key, "indices") == 0) {
        json_assert_type(&kv->value, JSON_WHOLE_NUMBER);
        primitive_out->indices = kv->value.data.whole_number;
    } else if (strcmp(kv->key, "mode") == 0) {
        json_assert_type(&kv->value, JSON_WHOLE_NUMBER);

        if (kv->value.data.whole_number >= NUM_PRIMITIVE_TYPES) {
            return -ERROR_INVALID_FORMAT;
        }

        primitive_out->mode = kv->value.data.whole_number;
    } else if (strcmp(kv->key, "material") == 0) {
        json_assert_type(&kv->value, JSON_WHOLE_NUMBER);
        primitive_out->material = kv->value.data.whole_number;
    } else {
        return -ERROR_INVALID_FORMAT;
    }

    return 0;
}

static int parse_primitive_json(
        const struct json_value *json, struct gltf_primitive *primitive_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (parse_primitive_properties(&kv, primitive_out) != 0) {
            return -ERROR_INVALID_FORMAT;
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
        vector_init(mesh.primitives, struct gltf_primitive);

        for (size_t i = 0; i < vector_size(mesh_json->data.object); i++) {
            struct key_value kv = mesh_json->data.object[i];

            if (strcmp(kv.key, "primitives") == 0) {
                json_assert_type(&kv.value, JSON_ARRAY);
                parse_multiple_json(&kv.value,
                        struct gltf_primitive,
                        mesh.primitives,
                        parse_primitive_json);
            } else if (strcmp(kv.key, "name") == 0) {
                json_assert_type(&kv.value, JSON_STRING);
            }
        }

        vector_append(*mesh_out, mesh);
    }

    return 0;
}

static int parse_float_json(const struct json_value *json, float *float_out) {
    json_assert_type(json, JSON_NUMBER);
    *float_out = json->data.number;
    return 0;
}

static int parse_whole_number_json(
        const struct json_value *json, size_t *whole_number_out) {
    json_assert_type(json, JSON_WHOLE_NUMBER);
    *whole_number_out = json->data.whole_number;
    return 0;
}

static int parse_node_json(
        const struct json_value *json, struct gltf_node *node_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "camera") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            node_out->camera = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "children") == 0) {
            parse_multiple_json(&kv.value,
                    size_t,
                    node_out->children,
                    parse_whole_number_json);
        } else if (strcmp(kv.key, "skin") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            node_out->skin = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "matrix") == 0) {
            parse_matn_json(kv.value, 4, node_out->matrix);
        } else if (strcmp(kv.key, "mesh") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            node_out->mesh = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "rotation") == 0) {
            parse_vecn_json(kv.value, 4, node_out->rotation);
        } else if (strcmp(kv.key, "scale") == 0) {
            parse_vecn_json(kv.value, 3, node_out->scale);
        } else if (strcmp(kv.key, "translation") == 0) {
            parse_vecn_json(kv.value, 3, node_out->translation);
        } else if (strcmp(kv.key, "weights") == 0) {
            parse_multiple_json(
                    &kv.value, float, node_out->weights, parse_float_json);
        }
    }

    return 0;
}

static int parse_scene_json(
        const struct json_value *json, struct gltf_scene *scene_out) {
    json_assert_type(json, JSON_OBJECT);

    vector_init(scene_out->nodes, size_t);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "nodes") == 0) {
            parse_multiple_json(&kv.value,
                    size_t,
                    scene_out->nodes,
                    parse_whole_number_json);
        }
    }

    return 0;
}

static int parse_animation_sampler_json(const struct json_value *json,
        struct gltf_animation_sampler *sampler_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "input") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            sampler_out->input = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "interpolation") == 0) {
            json_assert_type(&kv.value, JSON_STRING);

            if (strcmp(kv.value.data.string, "LINEAR") == 0) {
                sampler_out->interpolation = INTERPOLATION_LINEAR;
            } else if (strcmp(kv.value.data.string, "STEP") == 0) {
                sampler_out->interpolation = INTERPOLATION_STEP;
            } else if (strcmp(kv.value.data.string, "CUBICSPLINE") == 0) {
                sampler_out->interpolation = INTERPOLATION_CUBICSPLINE;
            } else {
                return -ERROR_INVALID_FORMAT;
            }
        } else if (strcmp(kv.key, "output") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            sampler_out->output = kv.value.data.whole_number;
        }
    }

    return 0;
}

static int parse_animation_channel_target_json(const struct json_value *json,
        struct gltf_animation_target *target_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "node") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            target_out->node = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "path") == 0) {
            json_assert_type(&kv.value, JSON_STRING);

            if (strcmp(kv.value.data.string, "translation") == 0) {
                target_out->path = ANIMATION_PATH_TRANSLATION;
            } else if (strcmp(kv.value.data.string, "rotation") == 0) {
                target_out->path = ANIMATION_PATH_ROTATION;
            } else if (strcmp(kv.value.data.string, "scale") == 0) {
                target_out->path = ANIMATION_PATH_SCALE;
            } else if (strcmp(kv.value.data.string, "weights") == 0) {
                target_out->path = ANIMATION_PATH_WEIGHTS;
            } else {
                return -ERROR_INVALID_FORMAT;
            }
        }
    }

    return 0;
}

static int parse_animation_channel_json(const struct json_value *json,
        struct gltf_animation_channel *channel_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "sampler") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            channel_out->sampler = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "target") == 0) {
            json_assert_type(&kv.value, JSON_OBJECT);

            if (parse_animation_channel_target_json(
                        &kv.value, &channel_out->target)) {
                return -ERROR_INVALID_FORMAT;
            }
        }
    }

    return 0;
}

static int parse_animation_json(
        const struct json_value *json, struct gltf_animation *animation_out) {
    json_assert_type(json, JSON_OBJECT);

    vector_init(animation_out->channels, struct gltf_animation_channel);
    vector_init(animation_out->samplers, struct gltf_animation_sampler);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "channels") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_animation_channel,
                    animation_out->channels,
                    parse_animation_channel_json);
        } else if (strcmp(kv.key, "samplers") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_animation_sampler,
                    animation_out->samplers,
                    parse_animation_sampler_json);
        } else if (strcmp(kv.key, "name") == 0) {
            json_assert_type(&kv.value, JSON_STRING);
            string_copy(animation_out->name, kv.value.data.string);
        }
    }

    return 0;
}

static int parse_image_json(
        const struct json_value *json, struct gltf_image *image_out) {
    json_assert_type(json, JSON_OBJECT);

    if (vector_size(json->data.object) != 1) {
        return -ERROR_INVALID_FORMAT;
    }

    struct key_value kv = json->data.object[0];

    if (strcmp(kv.key, "uri") != 0) {
        return -ERROR_INVALID_FORMAT;
    }

    json_assert_type(&kv.value, JSON_STRING);
    string_copy(image_out->uri, kv.value.data.string);

    return 0;
}

static int parse_texture_json(
        const struct json_value *json, struct gltf_texture *texture_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "sampler") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            texture_out->sampler = kv.value.data.whole_number;
        } else if (strcmp(kv.key, "source") == 0) {
            json_assert_type(&kv.value, JSON_WHOLE_NUMBER);
            texture_out->source = kv.value.data.whole_number;
        }
    }

    return 0;
}

static int parse_material_json(
        const struct json_value *json, struct gltf_material *material_out) {
    json_assert_type(json, JSON_OBJECT);

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "name") == 0) {
            json_assert_type(&kv.value, JSON_STRING);
            string_copy(material_out->name, kv.value.data.string);
        } else if (strcmp(kv.key, "pbrMetallicRoughness") == 0) {
            json_assert_type(&kv.value, JSON_OBJECT);

            for (size_t j = 0; j < vector_size(kv.value.data.object); j++) {
                struct key_value pbr_kv = kv.value.data.object[j];

                if (strcmp(pbr_kv.key, "baseColorFactor") == 0) {
                    json_assert_type(&pbr_kv.value, JSON_ARRAY);
                    parse_vecn_json(
                            pbr_kv.value, 4, material_out->base_color_factor);
                } else if (strcmp(pbr_kv.key, "metallicFactor") == 0) {
                    parse_float_json(
                            &pbr_kv.value, &material_out->metallic_factor);
                } else if (strcmp(pbr_kv.key, "roughnessFactor") == 0) {
                    parse_float_json(
                            &pbr_kv.value, &material_out->roughness_factor);
                }
            }
        }
    }

    return 0;
}

int parse_gltf_json(const struct json_value *json, struct gltf_file *file_out) {
    int retval = 0;

    if (json->type != JSON_OBJECT) {
        return -ERROR_INVALID_FORMAT;
    }

    for (size_t i = 0; i < vector_size(json->data.object); i++) {
        struct key_value kv = json->data.object[i];

        if (strcmp(kv.key, "accessors") == 0) {
            parse_multiple_json(&kv.value,
                    struct accessor,
                    file_out->accessors,
                    parse_accessor_json);
        } else if (strcmp(kv.key, "buffers") == 0) {
            if ((retval = parse_buffers_json(&kv.value, &file_out->buffers))) {
                return retval;
            }
        } else if (strcmp(kv.key, "bufferViews") == 0) {
            if ((retval = parse_buffer_views_json(
                         &kv.value, &file_out->buffer_views))) {
                return retval;
            }
        } else if (strcmp(kv.key, "meshes") == 0) {
            if ((retval = parse_meshes_json(&kv.value, &file_out->meshes))) {
                return retval;
            }
        } else if (strcmp(kv.key, "nodes") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_node,
                    file_out->nodes,
                    parse_node_json);
        } else if (strcmp(kv.key, "scenes") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_scene,
                    file_out->scenes,
                    parse_scene_json);
        } else if (strcmp(kv.key, "animations") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_animation,
                    file_out->animations,
                    parse_animation_json);
        } else if (strcmp(kv.key, "images") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_image,
                    file_out->images,
                    parse_image_json);
        } else if (strcmp(kv.key, "textures") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_texture,
                    file_out->textures,
                    parse_texture_json);
        } else if (strcmp(kv.key, "materials") == 0) {
            parse_multiple_json(&kv.value,
                    struct gltf_material,
                    file_out->materials,
                    parse_material_json);
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
        goto cleanup;
    }

    retval = parse_gltf_json(&root, file_out);

cleanup:
    munmap(buffer, file_size);
    return retval;
}

int gltf_load_file(char const *path, struct gltf_file *file_out) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return -ERROR_IO;
    }

    int retval = gltf_parse(file, file_out);

    fclose(file);
    return retval;
}
