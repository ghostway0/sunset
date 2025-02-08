#include <cglm/vec3.h>
#include <stdint.h>

#include "bitmask.h"
#include "cglm/mat4.h"
#include "images.h"
#include "internal/utils.h"
#include "sunset/camera.h"
#include "sunset/ecs.h"
#include "sunset/engine.h"
#include "sunset/input.h"
#include "sunset/rman.h"

typedef struct Clickable {
    void (*clicked_callback)(EngineContext *, EntityPtr clicked_entity);
} Clickable;

DECLARE_COMPONENT_ID(Clickable);

static void object_click_handler(EngineContext *ctx, void *, Event event) {
    uint32_t *focus = rman_get(&ctx->rman, RESOURCE_ID(input_focus));

    if (*focus != FOCUS_MAIN)
        return;

    Bitmask bitmask;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &bitmask);
    bitmask_set(&bitmask, COMPONENT_ID(Clickable));
    bitmask_set(&bitmask, COMPONENT_ID(Transform));

    WorldIterator it = worldit_create(&ctx->world, bitmask);

    while (worldit_is_valid(&it)) {
        EntityPtr eptr = worldit_get_entityptr(&it);

        Transform *transform = ecs_component_from_ptr(
                &ctx->world, eptr, COMPONENT_ID(Transform));
        Clickable *clickable = ecs_component_from_ptr(
                &ctx->world, eptr, COMPONENT_ID(Clickable));

        if (camera_is_over(&ctx->camera, transform->bounding_box, 0.05f)) {
            clickable->clicked_callback(ctx, eptr);
            break;
        }

        worldit_advance(&it);
    }

    worldit_destroy(&it);
}

typedef struct AxisGizmo {
    Index target_entity;
    vec3 axis;
} AxisGizmo;

DECLARE_COMPONENT_ID(AxisGizmo);

static void on_axis_clicked(EngineContext *ctx, EntityPtr arrow_entity) {
    AxisGizmo *gizmo = ecs_component_from_ptr(
            &ctx->world, arrow_entity, COMPONENT_ID(AxisGizmo));
    Transform *target_t = ecs_get_component(
            &ctx->world, gizmo->target_entity, COMPONENT_ID(Transform));

    unused(gizmo);
    unused(target_t);
    // drag_state.is_dragging = true;
    // drag_state.target_entity = gizmo->target_entity;
    // glm_vec3_copy(gizmo->axis, drag_state.axis);
    // glm_vec3_copy(target_t->position, drag_state.initial_pos);
}

static void on_mouse_drag(EngineContext *ctx, Event e) {
    unused(ctx);
    unused(e);
    // if (!drag_state.is_dragging) return;
    //
    // float delta_x = e->x - drag_state.initial_mouse_x;
    // float delta = delta_x * 0.01f; // Sensitivity adjustment
    //
    // Transform *t = ecs_get_component(&ctx->world,
    // drag_state.target_entity, COMPONENT_ID(Transform));
    // glm_vec3_muladds(drag_state.axis, delta, t->position);
}

// void spawn_axis_arrows(EngineContext *ctx, Index target) {
//     Transform *target_t =
//             ecs_get_component(&ctx->world, target,
//             COMPONENT_ID(Transform));
//
//     if (!target_t) {
//         return;
//     }
//
//     vec3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
//
//     for (int i = 0; i < 3; i++) {
//         EntityBuilder builder;
//         entity_builder_init(&builder, &ctx->world);
//
//         Transform arrow_t = *target_t;
//         glm_vec3_add(arrow_t.position, axes[i], arrow_t.position);
//
//         Renderable rend = {.commands = vector_create(Command)};
//         Command line_cmd = {.type = COMMAND_LINE,
//                 .data.line = {.from = target_t->position,
//                         .to = arrow_t.position}};
//         vector_append(rend.commands, line_cmd);
//
//         Clickable clk = {.clicked_callback = on_axis_clicked};
//         AxisGizmo gizmo = {.target_entity = target, .axis = axes[i]};
//
//         entity_builder_add_component(
//                 &builder, COMPONENT_ID(Transform), &arrow_t);
//         entity_builder_add_component(
//                 &builder, COMPONENT_ID(Renderable), &rend);
//         entity_builder_add_component(
//                 &builder, COMPONENT_ID(Clickable), &clk);
//         entity_builder_add_component(
//                 &builder, COMPONENT_ID(AxisGizmo), &gizmo);
//         entity_builder_finish(&builder);
//     }
// }

static void player_controller_handler(
        EngineContext *engine_context, void *, Event event) {
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (*focus != FOCUS_MAIN) {
        return;
    }

    Key *key = (Key *)event.data;
    Camera *camera = &engine_context->camera;

    vec3 direction;

    switch (*key) {
        case KEY_W:
            glm_vec3_copy(camera->direction, direction);
            break;
        case KEY_S:
            glm_vec3_negate_to(camera->direction, direction);
            break;
        case KEY_A:
            glm_vec3_negate_to(camera->right, direction);
            break;
        case KEY_D:
            glm_vec3_copy(camera->right, direction);
            break;
        case KEY_U:
            glm_vec3_copy(camera->up, direction);
            break;
        case KEY_N:
            glm_vec3_negate_to(camera->up, direction);
            break;
        case KEY_ESCAPE:
            *focus = FOCUS_UI;
            backend_show_mouse(&engine_context->render_context);
            return;
        default:
            return;
    }

    camera_move_scaled(camera, direction, engine_context->dt);
}

DECLARE_RESOURCE_ID(axis_arrow);

void spawn_axis_arrows(EngineContext *ctx, Index target) {
    Transform *target_t =
            ecs_get_component(&ctx->world, target, COMPONENT_ID(Transform));
    if (!target_t)
        return;

    // uint32_t *arrow_mesh_id = rman_get(&ctx->rman, RESOURCE_ID(axis_arrow));
    //
    // struct {
    //     vec3 axis;
    //     Color color;
    // } axes[] = {{color_from_rgb(255, 0, 0)},
    //         {GLM_YUP, color_from_rgb(0, 255, 0)},
    //         {GLM_ZUP, color_from_rgb(0, 0, 255)}};
    //
    // uint32_t texture = 0;
    //
    // for (int i = 0; i < 3; i++) {
    //     EntityBuilder builder;
    //     entity_builder_init(&builder, &ctx->world);
    //
    //     // Copy target position and add slight offset
    //     Transform arrow_t;
    //     glm_vec3_copy(target_t->position, arrow_t.position);
    //     glm_vec3_add(arrow_t.position, axes[i].axis, arrow_t.position);
    //     glm_vec3_copy(GLM_VEC3_ZERO, arrow_t.rotation);
    //     arrow_t.scale = 0.2f; // Scale down arrows
    //
    //     // Create renderable component with mesh
    //     Renderable rend;
    //     vector_init(rend.commands);
    //
    //     Command mesh_cmd = {.type = COMMAND_MESH,
    //             .data.mesh = {.mesh_id = *arrow_mesh_id,
    //                     .transform = GLM_MAT4_IDENTITY_INIT,
    //                     .texture_id = texture}};
    //
    //     // Position mesh at arrow location
    //     glm_translate(mesh_cmd.data.mesh.transform, arrow_t.position);
    //     glm_scale_uni(mesh_cmd.data.mesh.transform, arrow_t.scale);
    //
    //     vector_append(rend.commands, mesh_cmd);
    //
    //     // Set up clickable component
    //     Clickable clk;
    //     clk.clicked_callback = on_axis_clicked;
    //
    //     // Set up axis gizmo component
    //     AxisGizmo gizmo;
    //     gizmo.target_entity = target;
    //     glm_vec3_copy(axes[i].axis, gizmo.axis);
    //
    //     // Add components
    //     entity_builder_add_component(
    //             &builder, COMPONENT_ID(Transform), &arrow_t);
    //     entity_builder_add_component(
    //             &builder, COMPONENT_ID(Renderable), &rend);
    //     entity_builder_add_component(
    //             &builder, COMPONENT_ID(Clickable), &clk);
    //     entity_builder_add_component(
    //             &builder, COMPONENT_ID(AxisGizmo), &gizmo);
    //
    //     entity_builder_finish(&builder);
    // }
}

int plugin_load(EngineContext *engine_context) {
    REGISTER_COMPONENT(&engine_context->world, Clickable);

    return 0;
}
