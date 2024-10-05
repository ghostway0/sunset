#include <math.h>
#include <string.h>

#include <cglm/cam.h>
#include <cglm/vec3.h>

#include "sunset/camera.h"
#include "sunset/math.h"
#include "sunset/utils.h"

void camera_init(struct camera *camera,
        struct camera_state state,
        struct camera_options options) {
    memcpy(&camera->position, &state.position, sizeof(vec3));
    memcpy(&camera->up, &state.up, sizeof(vec3));
    camera->yaw = state.yaw;
    camera->pitch = state.pitch;

    glm_vec3_normalize_to(
            (vec3){
                    cosf(state.yaw) * cosf(state.pitch),
                    sinf(state.pitch),
                    sinf(state.yaw) * cosf(state.pitch),
            },
            camera->direction);

    glm_vec3_cross(camera->up, camera->direction, camera->right);
    glm_vec3_cross(camera->direction, camera->right, camera->up);

    camera->fov = options.fov;
    camera->sensitivity = options.sensitivity;
    camera->speed = options.speed;
}

void camera_rotate_absolute(
        struct camera *camera, float x_angle, float y_angle) {
    camera->yaw += x_angle;
    camera->pitch += y_angle;

    camera->pitch = clamp(camera->pitch, -M_PI_2, M_PI_2);
    camera->yaw = fmodf(camera->yaw, 2 * M_PI);

    glm_vec3_rotate(
            camera->direction, x_angle * camera->sensitivity, camera->up);

    glm_vec3_rotate(
            camera->direction, y_angle * camera->sensitivity, camera->right);
    glm_vec3_rotate(camera->up, y_angle * camera->sensitivity, camera->right);

    glm_vec3_normalize(camera->direction);
    glm_vec3_cross(camera->direction, camera->up, camera->right);

    glm_vec3_normalize(camera->right);
    glm_vec3_normalize(camera->up);
}

void camera_rotate_scaled(struct camera *camera, float x_angle, float y_angle) {
    camera_rotate_absolute(camera,
            x_angle * camera->sensitivity,
            y_angle * camera->sensitivity);
}

// camera direction to world space
void camera_vec_to_world(struct camera *camera, vec3 direction) {
    glm_vec3_rotate(direction, -camera->yaw, camera->up);
    glm_vec3_rotate(direction, -camera->pitch, camera->right);
}

void camera_move(struct camera *camera, vec3 direction) {
    glm_vec3_scale(direction, camera->speed, direction);
    glm_vec3_add(camera->position, direction, camera->position);
}

void camera_recalculate_vectors(struct camera *camera) {
    glm_vec3_normalize(camera->direction);
    glm_vec3_cross(camera->direction, camera->right, camera->up);
    glm_vec3_normalize(camera->up);
    glm_vec3_cross(camera->up, camera->direction, camera->right);
    glm_vec3_normalize(camera->right);
}

// world->camera transformation matrix
void camera_get_view_matrix(struct camera *camera, mat4 dest) {
    vec3 center;

    glm_vec3_add(camera->position, camera->direction, center);
    glm_lookat(camera->position, center, camera->up, dest);
}

// camera->clip transformation matrix
void camera_get_projection_matrix(
        struct camera *camera, float aspect, mat4 dest) {
    glm_perspective(camera->fov, aspect, 0.1f, 100.0f, dest);
}
