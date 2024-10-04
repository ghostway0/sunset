#pragma once

#include <cglm/vec3.h>

struct camera_state {
    vec3 position;
    vec3 up;
    float yaw;
    float pitch;
};

struct camera_options {
    float fov;
    float speed;
    float sensitivity;
};

struct camera {
    vec3 position;
    vec3 direction;
    vec3 up;
    vec3 right;
    vec3 world_up;
    float yaw;
    float pitch;
    float fov;
    float speed;
    float sensitivity;
};
