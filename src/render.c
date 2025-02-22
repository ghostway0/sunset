#include <cglm/affine.h>
#include <cglm/mat4.h>

#include "internal/utils.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/ecs.h"
#include "sunset/engine.h"
#include "sunset/events.h"

#include "sunset/render.h"

DECLARE_COMPONENT_ID(TransformGraph);
DECLARE_COMPONENT_ID(Transform);
DECLARE_COMPONENT_ID(Renderable);

void render_setup(EngineContext *engine_context) {
    REGISTER_COMPONENT(&engine_context->world, Transform);
    REGISTER_COMPONENT(&engine_context->world, Renderable);
}

void calculate_model_matrix(Transform const *transform, mat4 model_matrix) {
    glm_mat4_identity(model_matrix);

    // HACK:
    vec3 position;
    memcpy(position, transform->position, sizeof(transform->position));

    glm_translate(model_matrix, position);

    // HACK:
    float angle = glm_vec3_norm((float *)transform->rotation);
    if (angle > EPSILON) {
        vec3 axis;
        glm_vec3_normalize_to((float *)transform->rotation, axis);
        glm_rotate(model_matrix, angle, axis);
    }

    glm_scale_uni(model_matrix, transform->scale);
}

void render_world(
        World const *world, Camera const *camera, CommandBuffer *cmdbuf) {
    Bitmask mask;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &mask);
    bitmask_set(&mask, COMPONENT_ID(Renderable));

    WorldIterator it = worldit_create(world, mask);

    while (worldit_is_valid(&it)) {
        Renderable *renderable =
                worldit_get_component(&it, COMPONENT_ID(Renderable));
        Transform *transform =
                worldit_get_component(&it, COMPONENT_ID(Transform));

        bool visible = true;

        if (transform) {
            // HACK: when I transition to my own math library, const
            // when unmutable would be a rule
            visible = camera_box_within_frustum(
                    (Camera *)camera, transform->bounding_box);
        }
        visible = true;

        if (visible) {
            cmdbuf_add_multiple(cmdbuf,
                    renderable->commands,
                    vector_size(renderable->commands));
        }

        worldit_advance(&it);
    }
}
