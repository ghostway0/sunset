#include <cglm/affine.h>
#include <cglm/mat4.h>

#include "internal/utils.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/ecs.h"
#include "sunset/engine.h"
#include "sunset/events.h"

#include "sunset/render.h"

DECLARE_COMPONENT_ID(Transform);
DECLARE_COMPONENT_ID(Renderable);

void render_setup(EngineContext *engine_context) {
    REGISTER_COMPONENT(&engine_context->world, Transform);
    REGISTER_COMPONENT(&engine_context->world, Renderable);
}

void calculate_model_matrix(
        World *world, EntityPtr entity, mat4 model_matrix) {
    glm_mat4_identity(model_matrix);

    EntityPtr current = entity;

    while (true) {
        Transform *t = ecs_component_from_ptr(
                world, current, COMPONENT_ID(Transform));

        mat4 local;
        glm_mat4_identity(local);

        glm_translate(local, t->position);

        // HACK:
        float angle = glm_vec3_norm((float *)t->rotation);
        if (angle > EPSILON) {
            vec3 axis;
            glm_vec3_normalize_to((float *)t->rotation, axis);
            glm_rotate(local, angle, axis);
        }

        glm_scale_uni(local, t->scale);

        glm_mul(local, model_matrix, model_matrix);

        if (eptr_eql(t->parent, ENTITY_PTR_INVALID)) {
            break;
        }

        current = t->parent;
    }
}

void render_world(World /*const*/ *world,
        Camera const *camera,
        CommandBuffer *cmdbuf) {
    Bitmask mask;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &mask);
    bitmask_set(&mask, COMPONENT_ID(Renderable));

    WorldIterator it = worldit_create(world, mask);

    while (worldit_is_valid(&it)) {
        Renderable *renderable =
                worldit_get_component(&it, COMPONENT_ID(Renderable));
        Transform *transform =
                worldit_get_component(&it, COMPONENT_ID(Transform));
        EntityPtr eptr = worldit_get_entityptr(&it);

        bool visible = true;

        if (transform) {
            // HACK: when I transition to my own math library, const
            // when unmutable would be a rule
            visible = camera_box_within_frustum(
                    (Camera *)camera, transform->bounding_box);

            // if (transform->dirty) {
            calculate_model_matrix(world, eptr, renderable->context.model);
            // }
        }

        if (visible) {
            cmdbuf_add_multiple(cmdbuf,
                    renderable->commands,
                    vector_size(renderable->commands),
                    &renderable->context);
        }

        worldit_advance(&it);
    }
}

void entity_move(World *world, EntityPtr eptr, vec3 offset) {
    Transform *t =
            ecs_component_from_ptr(world, eptr, COMPONENT_ID(Transform));

    glm_vec3_add(t->position, offset, t->position);
    aabb_translate(&t->bounding_box, offset);

    if (!t->children) {
        return;
    }

    for (size_t i = 0; i < vector_size(t->children); i++) {
        entity_move(world, t->children[i], offset);
    }
}
