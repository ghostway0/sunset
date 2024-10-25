#include "sunset/geometry.h"
#include <GLFW/glfw3.h>

// backend-specific data
struct render_context {
    size_t width, height;
    GLFWwindow *window;
    GLFWcursor *cursor;
};

struct compiled_mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    size_t num_indices;
};

struct compiled_texture {
    GLuint tex;
};

int compile_mesh(struct mesh const *mesh, struct compiled_mesh *mesh_out);

void backend_draw_mesh(struct compiled_mesh *mesh);

void backend_draw(struct render_context *context);

void backend_free(struct render_context *context);

void backend_free_program(struct program *program);

void compiled_mesh_destroy(struct compiled_mesh *mesh);

