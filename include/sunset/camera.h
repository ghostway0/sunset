#pragma once

#include "sunset/geometry.h"
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
    float aspect_ratio;
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
    float aspect_ratio;
    mat4 view_matrix;
    mat4 projection_matrix;
};

void camera_init(struct camera_state state,
        struct camera_options options,
        struct camera *camera_out);

void camera_rotate_absolute(
        struct camera *camera, float x_angle, float y_angle);

void camera_rotate_scaled(struct camera *camera, float x_angle, float y_angle);

void camera_move(struct camera *camera, vec3 direction);

void camera_recalculate_vectors(struct camera *camera);

void camera_vec_to_world(struct camera *camera, vec3 direction);

bool camera_point_in_frustum(struct camera *camera, vec3 point);

bool camera_box_within_frustum(struct camera *camera, struct box box);
