#include "sunset/geometry.h"
#include "sunset/shader.h"
#include "sunset/vector.h"
#include <GLFW/glfw3.h>
#include "sunset/map.h"

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
    // GLuint framebuffer;
    // GLuint texture;
    // GLuint depthbuffer;
    map(struct instanced_mesh) instanced_meshes;
    struct instancing_buffer *current_instancing_buffer;
};

enum backend_program_type {
    PROGRAM_DRAW_INSTANCED_MESH,
    PROGRAM_DRAW_MESH,
    PROGRAM_DRAW_DIRECT_LIGHT,
    NUM_BACKEND_PROGRAMS,
};

struct instancing_buffer {
    uint32_t mesh_id;
    vector(mat4) transforms;
    vector(uint32_t) required_textures;
    GLuint textures_buffer;
    GLuint transforms_buffer;
};

// backend-specific data
struct render_context {
    size_t width, height;
    GLFWwindow *window;
    GLFWcursor *cursor;

    struct frame_cache frame_cache;
    struct program backend_programs[NUM_BACKEND_PROGRAMS];
    vector(struct compiled_mesh) meshes;
    vector(struct compiled_texture) textures;
};

uint32_t backend_register_mesh(struct render_context *context, struct mesh mesh);

void backend_draw_mesh(struct compiled_mesh *mesh);

void backend_draw(struct render_context *context, mat4 view, mat4 projection);

void backend_free(struct render_context *context);

void backend_free_program(struct program *program);

void compiled_mesh_destroy(struct compiled_mesh *mesh);

