#include <GLFW/glfw3.h>

// backend-specific data
struct render_context {
    size_t width, height;
    GLFWwindow *window;
    GLFWcursor *cursor;
};
