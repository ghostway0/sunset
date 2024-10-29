#include <log.h>
#include <string.h>

#include <cglm/cam.h>
#include <cglm/vec3.h>

#include "sunset/camera.h"
#include "sunset/math.h"

// world->camera transformation matrix
static void calculate_view_matrix(struct camera *camera, mat4 dest) {
    vec3 center;

    glm_vec3_add(camera->position, camera->direction, center);
    glm_lookat(camera->position, center, camera->up, dest);
}

// camera->clip transformation matrix
static void calculate_projection_matrix(
        const struct camera *camera, float aspect, mat4 dest) {
    glm_perspective(camera->fov, aspect, 0.1f, 100.0f, dest);
}

void camera_init(struct camera_state state,
        struct camera_options options,
        struct camera *camera_out) {
    memcpy(&camera_out->position, &state.position, sizeof(vec3));
    memcpy(&camera_out->up, &state.up, sizeof(vec3));
    camera_out->yaw = state.yaw;
    camera_out->pitch = state.pitch;

    glm_vec3_copy(
            (vec3){
                    sinf(state.pitch),
                    -sinf(state.yaw) * cosf(state.pitch),
                    -cosf(state.yaw) * cosf(state.pitch),
            },
            camera_out->direction);

    glm_vec3_cross(camera_out->up, camera_out->direction, camera_out->right);
    glm_vec3_cross(camera_out->direction, camera_out->right, camera_out->up);

    camera_out->fov = options.fov;
    camera_out->sensitivity = options.sensitivity;
    camera_out->speed = options.speed;
    camera_out->aspect_ratio = options.aspect_ratio;

    calculate_view_matrix(camera_out, camera_out->view_matrix);
    calculate_projection_matrix(camera_out,
            camera_out->aspect_ratio,
            camera_out->projection_matrix);
}

void camera_rotate_absolute(
        struct camera *camera, float x_angle, float y_angle) {
    camera->yaw += x_angle;
    camera->pitch += y_angle;

    camera->pitch = clamp(camera->pitch, -GLM_PI_2, GLM_PI_2);
    camera->yaw = fmodf(camera->yaw, 2 * GLM_PI);

    glm_vec3_rotate(
            camera->direction, x_angle * camera->sensitivity, camera->up);

    glm_vec3_rotate(
            camera->direction, y_angle * camera->sensitivity, camera->right);
    glm_vec3_rotate(camera->up, y_angle * camera->sensitivity, camera->right);

    glm_vec3_normalize(camera->direction);
    glm_vec3_cross(camera->direction, camera->up, camera->right);

    glm_vec3_normalize(camera->right);
    glm_vec3_normalize(camera->up);

    calculate_view_matrix(camera, camera->view_matrix);
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

    calculate_view_matrix(camera, camera->view_matrix);
}

void camera_recalculate_vectors(struct camera *camera) {
    glm_vec3_normalize(camera->direction);
    glm_vec3_cross(camera->direction, camera->right, camera->up);
    glm_vec3_normalize(camera->up);
    glm_vec3_cross(camera->up, camera->direction, camera->right);
    glm_vec3_normalize(camera->right);
    calculate_view_matrix(camera, camera->view_matrix);
    calculate_projection_matrix(
            camera, camera->aspect_ratio, camera->projection_matrix);
}

bool camera_point_in_frustum(struct camera *camera, vec3 point) {
    vec4 clip = {point[0], point[1], point[2], 1.0f};

    mat4 view_projection;
    glm_mat4_mul(
            camera->projection_matrix, camera->view_matrix, view_projection);

    glm_mat4_mulv(view_projection, clip, clip);

    clip[0] /= clip[3];
    clip[1] /= clip[3];
    clip[2] /= clip[3];

    return clip[0] >= -1.0f && clip[0] <= 1.0f && clip[1] >= -1.0f
            && clip[1] <= 1.0f && clip[2] >= -1.0f && clip[2] <= 1.0f;
}

bool camera_box_within_frustum(struct camera *camera, struct box box) {
    vec3 points[8] = {
            {box.min[0], box.min[1], box.min[2]},
            {box.min[0], box.min[1], box.max[2]},
            {box.min[0], box.max[1], box.min[2]},
            {box.min[0], box.max[1], box.max[2]},
            {box.max[0], box.min[1], box.min[2]},
            {box.max[0], box.min[1], box.max[2]},
            {box.max[0], box.max[1], box.min[2]},
            {box.max[0], box.max[1], box.max[2]},
    };

    for (size_t i = 0; i < 8; i++) {
        if (camera_point_in_frustum(camera, points[i])) {
            return true;
        }
    }

    return false;
}
