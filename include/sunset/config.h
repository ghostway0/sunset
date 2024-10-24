#pragma once

#include <GLFW/glfw3.h>

enum shader_type {
    SHADER_VERTEX = GL_VERTEX_SHADER,
    SHADER_FRAGMENT = GL_FRAGMENT_SHADER,
    SHADER_GEOMETRY = GL_GEOMETRY_SHADER,
    SHADER_TESSELLATION_CONTROL = GL_TESS_CONTROL_SHADER,
    SHADER_TESSELLATION_EVALUATION = GL_TESS_EVALUATION_SHADER,
};