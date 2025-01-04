#pragma once

#include <cglm/vec3.h>

typedef struct AABB AABB;

struct CameraState {
    vec3 position;
    vec3 up;
    float yaw;
    float pitch;
} typedef CameraState;

struct CameraOptions {
    // in radians
    float fov;
    float speed;
    float sensitivity;
    float aspect_ratio;
} typedef CameraOptions;

struct Camera {
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
} typedef Camera;

void camera_init(
        CameraState state, CameraOptions options, Camera *camera_out);

void camera_rotate_absolute(Camera *camera, float x_angle, float y_angle);

void camera_rotate_scaled(Camera *camera, float x_angle, float y_angle);

void camera_move_absolute(Camera *camera, vec3 direction);

void camera_move_scaled(Camera *camera, vec3 direction);

void camera_recalculate_vectors(Camera *camera);

void camera_vec_to_world(Camera *camera, vec3 direction);

bool camera_point_in_frustum(Camera *camera, vec3 point);

bool camera_box_within_frustum(Camera *camera, AABB aabb);

void camera_set_rotation(Camera *camera, float x_angle, float y_angle);
