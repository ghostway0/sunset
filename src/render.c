#include <cglm/affine.h>
#include <cglm/mat4.h>

#include "sunset/render.h"

void calculate_model_matrix(Transform const *transform, mat4 model_matrix) {
    glm_mat4_identity(model_matrix);

    // HACK:
    vec3 position;
    memcpy(position, transform->position, sizeof(transform->position));

    glm_translate(model_matrix, position);

    glm_rotate(
            model_matrix, transform->rotation[0], (vec3){1.0f, 0.0f, 0.0f});
    glm_rotate(
            model_matrix, transform->rotation[1], (vec3){0.0f, 1.0f, 0.0f});
    glm_rotate(
            model_matrix, transform->rotation[2], (vec3){0.0f, 0.0f, 1.0f});
}
