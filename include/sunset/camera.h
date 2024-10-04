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

void camera_init(struct camera *camera,
        struct camera_state state,
        struct camera_options options);

void camera_rotate(struct camera *camera, float x_angle, float y_angle);

void camera_move(struct camera *camera, vec3 direction);

void camera_recalculate_vectors(struct camera *camera);

void camera_get_view_matrix(struct camera *camera, mat4 dest);

void camera_to_world(struct camera *camera, vec3 direction);
