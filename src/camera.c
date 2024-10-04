#include <math.h>
#include <string.h>

#include <cglm/vec3.h>

#include "sunset/camera.h"

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
