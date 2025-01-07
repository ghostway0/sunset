#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sunset/byte_stream.h"
#include "sunset/render.h"
#include "sunset/shader.h"
#include "sunset/input.h"

#ifdef SUNSET_BACKEND_OPENGL

#include "opengl_backend.h"

#endif

struct ssbo_argument {
    char const *name;
    struct byte_stream *data;
};

struct shader_arguments {
    struct uniform_argument *uniforms;
    size_t num_uniforms;
    struct ssbo_argument *ssbos;
    size_t num_ssbos;
};

int backend_setup(RenderContext *context, RenderConfig config);

int backend_create_program(struct program *program_out);

int backend_program_add_shader(struct program *program,
        char const *source,
        enum shader_type shader_type);

int backend_link_program(struct program *program);

int backend_set_program_arguments(struct program *program,
        struct active_argument *arguments,
        size_t num_arguments);

int backend_setup_shader(struct program *program,
        struct active_argument *arguments,
        size_t num_arguments);

struct instanced_mesh *backend_create_instanced_mesh(
        RenderContext *context, struct compiled_mesh const *compiled_mesh);

void backend_set_user_context(RenderContext *context, void *user_context);

int backend_register_texture_atlas(RenderContext *context,
        struct image const *atlas_image,
        struct rect *bounds,
        size_t num_textures,
        uint32_t *first_id_out);

struct render_config backend_build_render_config(char const *title);

bool backend_should_stop(RenderContext *context);

void backend_hide_mouse(RenderContext *context);

void backend_show_mouse(RenderContext *context);

InputState backend_capture_input(RenderContext *context);
