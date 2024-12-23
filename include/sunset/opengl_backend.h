#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/types.h>

#include "sunset/commands.h"
#include "sunset/geometry.h"
#include "sunset/map.h"
#include "sunset/shader.h"
#include "sunset/vector.h"

struct event_queue;

enum shader_type {
    SHADER_VERTEX = GL_VERTEX_SHADER,
    SHADER_FRAGMENT = GL_FRAGMENT_SHADER,
    SHADER_GEOMETRY = GL_GEOMETRY_SHADER,
    SHADER_TESSELLATION_CONTROL = GL_TESS_CONTROL_SHADER,
    SHADER_TESSELLATION_EVALUATION = GL_TESS_EVALUATION_SHADER,
};

struct compiled_mesh {
    uint32_t id;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    size_t num_indices;
    GLenum tex;
};

struct instanced_mesh {
    uint32_t id;
    struct compiled_mesh mesh;
    GLuint transform_buffer;
};

struct frame_cache {
    map(struct instancing_buffer) instancing_buffers;
    struct instancing_buffer *current_instancing_buffer;

    mat4 model_matrix;
    mat4 view_matrix;
    mat4 projection_matrix;
};

enum backend_program_type {
    PROGRAM_DRAW_INSTANCED_MESH,
    PROGRAM_DRAW_MESH,
    PROGRAM_DRAW_DIRECT_LIGHT,
    PROGRAM_DRAW_TEXT,
    PROGRAM_DRAW_DIRECT,
    NUM_BACKEND_PROGRAMS,
};

struct instancing_buffer {
    uint32_t mesh_id;
    uint32_t atlas_id;
    vector(mat4) transforms;
};

struct compiled_texture {
    uint32_t atlas_id;
    struct rect bounds;
};

struct atlas {
    GLuint buffer;
    size_t current_size;
};

struct compiled_font {
    uint32_t id;
    GLuint vao;
    GLuint vbo;

    GLuint *glyph_tex;
    size_t num_glyphs;
};

// backend-specific data
struct render_context {
    size_t screen_width, screen_height;
    GLFWwindow *window;
    GLFWcursor *cursor;

    mat4 ortho_projection;

    struct frame_cache frame_cache;
    struct program backend_programs[NUM_BACKEND_PROGRAMS];
    vector(struct compiled_mesh) meshes;
    vector(struct compiled_texture) textures;
    vector(struct atlas) atlases;
    GLuint texture_atlas;

    struct point mouse;
    bool first_mouse;

    struct event_queue *event_queue;

    struct command_buffer command_buffer;
};

uint32_t backend_register_mesh(
        struct render_context *context, struct mesh mesh);

void backend_draw_mesh(struct compiled_mesh *mesh);

void backend_draw(struct render_context *context,
        struct command_buffer *command_buffer,
        mat4 view,
        mat4 projection);

void backend_destroy(struct render_context *context);

void backend_destroy_program(struct program *program);

void compiled_mesh_destroy(struct compiled_mesh *mesh);
