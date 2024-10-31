#include <string.h>

#include <cglm/cam.h>
#include <cglm/vec3.h>

#include "cglm/quat.h"
#include "cglm/types.h"
#include "log.h"
#include "sunset/camera.h"
#include "sunset/math.h"
#include "sunset/utils.h"

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

    camera_set_rotation(camera_out, camera_out->yaw, camera_out->pitch);

    camera_out->fov = options.fov;
    camera_out->sensitivity = options.sensitivity;
    camera_out->speed = options.speed;
    camera_out->aspect_ratio = options.aspect_ratio;

    calculate_projection_matrix(camera_out,
            camera_out->aspect_ratio,
            camera_out->projection_matrix);
}

void camera_rotate_absolute(
        struct camera *camera, float x_angle, float y_angle) {
    // NOTE: when pitch is clamped, camera is flipped
    camera->pitch = clamp(camera->pitch + y_angle, -GLM_PI_2, GLM_PI_2);
    camera->yaw = fmodf(camera->yaw + x_angle, 2 * GLM_PI);

    versor yaw_quat;
    glm_quatv(yaw_quat, x_angle, camera->world_up);

    versor pitch_quat;
    glm_quatv(pitch_quat, y_angle, camera->right);

    versor rotation_quat;
    glm_quat_mul(yaw_quat, pitch_quat, rotation_quat);

    glm_quat_rotatev(rotation_quat, camera->direction, camera->direction);
    glm_quat_rotatev(rotation_quat, camera->up, camera->up);

    glm_vec3_cross(camera->direction, camera->up, camera->right);
    glm_vec3_normalize(camera->right);
    glm_vec3_normalize(camera->up);

    calculate_view_matrix(camera, camera->view_matrix);
}

void camera_set_rotation(struct camera *camera, float x_angle, float y_angle) {
    camera->yaw = x_angle;
    camera->pitch = y_angle;

    camera->pitch = clamp(camera->pitch, -GLM_PI_2, GLM_PI_2);
    camera->yaw = fmodf(camera->yaw, 2 * GLM_PI);

    glm_vec3_copy(
            (vec3){
                    sinf(camera->pitch),
                    -sinf(camera->yaw) * cosf(camera->pitch),
                    -cosf(camera->yaw) * cosf(camera->pitch),
            },
            camera->direction);

    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, camera->world_up);

    glm_vec3_copy((vec3){0.0f, 1.0f, 0.0f}, camera->up);

    glm_vec3_cross(camera->up, camera->direction, camera->right);
    glm_vec3_cross(camera->direction, camera->right, camera->up);

    glm_vec3_normalize(camera->direction);
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
    glm_vec3_rotate(direction, camera->yaw, camera->world_up);
    glm_vec3_rotate(direction, camera->pitch, camera->right);
}

void camera_move(struct camera *camera, vec3 direction) {
    glm_vec3_scale(direction, camera->speed, direction);
    camera_vec_to_world(camera, direction);

    glm_vec3_add(camera->position, direction, camera->position);

    calculate_view_matrix(camera, camera->view_matrix);
}

void camera_recalculate_vectors(struct camera *camera) {
    camera_set_rotation(camera, camera->yaw, camera->pitch);
    calculate_projection_matrix(
            camera, camera->aspect_ratio, camera->projection_matrix);
}

bool camera_point_in_frustum(struct camera *camera, vec3 point) {
    vec4 quat_point = {point[0], point[1], point[2], 1.0f};
    mat4 view_projection;
    vec4 clip;

    glm_mat4_mul(
            camera->projection_matrix, camera->view_matrix, view_projection);
    glm_mat4_mulv(view_projection, quat_point, clip);

    return clip[0] >= -clip[3] && clip[0] <= clip[3] && clip[1] >= -clip[3]
            && clip[1] <= clip[3] && clip[2] >= -clip[3] && clip[2] <= clip[3];
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
