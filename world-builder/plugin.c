#include <stdint.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>
#include <log.h>

#include "sunset/bitmask.h"
#include "sunset/camera.h"
#include "sunset/ecs.h"
#include "sunset/engine.h"
#include "sunset/events.h"
#include "sunset/geometry.h"
#include "sunset/images.h"
#include "sunset/input.h"
#include "sunset/obj_file.h"
#include "sunset/render.h"
#include "sunset/rman.h"
#include "sunset/vector.h"
#include "sunset/vfs.h"

typedef struct Clickable {
    void (*clicked_callback)(EngineContext *, EntityPtr clicked_entity);
} Clickable;

DECLARE_COMPONENT_ID(Clickable);

static void object_click_handler(EngineContext *engine_context,
        void * /*local_context*/,
        Event /*event*/) {
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (*focus != FOCUS_MAIN) {
        return;
    }

    Bitmask bitmask;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &bitmask);
    bitmask_set(&bitmask, COMPONENT_ID(Clickable));
    bitmask_set(&bitmask, COMPONENT_ID(Transform));

    WorldIterator it = worldit_create(&engine_context->world, bitmask);
    while (worldit_is_valid(&it)) {
        EntityPtr eptr = worldit_get_entityptr(&it);

        Transform *transform = ecs_component_from_ptr(
                &engine_context->world, eptr, COMPONENT_ID(Transform));
        Clickable *clickable = ecs_component_from_ptr(
                &engine_context->world, eptr, COMPONENT_ID(Clickable));

        if (camera_crosshair_over(
                    &engine_context->camera, &transform->bounding_box)
                && clickable->clicked_callback) {
            clickable->clicked_callback(engine_context, eptr);
        }

        worldit_advance(&it);
    }

    worldit_destroy(&it);
}

DECLARE_COMPONENT_ID(AxisGizmo);

static void player_controller_handler_mouse(
        EngineContext *engine_context, void *, Event const event) {
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (*focus != FOCUS_MAIN) {
        return;
    }

    MouseMoveEvent *mouse_move = (MouseMoveEvent *)event.data;

    camera_rotate_scaled(&engine_context->camera,
            mouse_move->offset.x,
            mouse_move->offset.y);
}

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
        case KEY_A:
            glm_vec3_negate_to(camera->right, direction);
            break;
        case KEY_S:
            glm_vec3_negate_to(camera->direction, direction);
            break;
        case KEY_D:
            glm_vec3_copy(camera->right, direction);
            break;
        case KEY_ESCAPE:
            *focus = FOCUS_NULL;
            backend_show_mouse(&engine_context->render_context);
            return;
        default:
            return;
    }

    camera_move_scaled(camera, direction, engine_context->dt);
}

DECLARE_RESOURCE_ID(axis_arrow_mesh);

DECLARE_RESOURCE_ID(axis_arrow_texture1);
DECLARE_RESOURCE_ID(axis_arrow_texture2);
DECLARE_RESOURCE_ID(axis_arrow_texture3);

typedef struct AxisArrow {
    EntityPtr parent;
} AxisArrow;

DECLARE_COMPONENT_ID(AxisArrow);

static void thing2(
        EngineContext *engine_context, void *, Event const event) {
    MouseClickEvent *click = (MouseClickEvent *)event.data;
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (click->button == MOUSE_BUTTON_LEFT
            && one_matches(*focus, FOCUS_NULL, FOCUS_NULL)) {
        *focus = FOCUS_MAIN;
        backend_hide_mouse(&engine_context->render_context);
    }
}

void spawn_axis_arrow(EngineContext *engine_context,
        Transform const *transform,
        EntityPtr parent,
        vec3 axis,
        uint32_t texture) {
    Renderable rend;
    vector_init(rend.commands);

    Command mesh_cmd = {.type = COMMAND_MESH,
            .mesh = {.mesh_id = 0,
                    .transform = {},
                    .texture_id = texture}};

    Transform new_transform = *transform;
    glm_vec3_add(new_transform.position, axis, new_transform.position);

    calculate_model_matrix(&new_transform, mesh_cmd.mesh.transform);
    vector_append(rend.commands, mesh_cmd);

    AxisArrow arrow = {.parent = parent};

    EntityBuilder builder;
    entity_builder_init(&builder, &engine_context->world);
    entity_builder_add(&builder, COMPONENT_ID(Transform), &new_transform);
    entity_builder_add(&builder, COMPONENT_ID(Renderable), &rend);
    entity_builder_add(&builder, COMPONENT_ID(AxisArrow), &arrow);
    entity_builder_finish(&builder);
}

void spawn_axis_arrows(EngineContext *engine_context, EntityPtr target) {
    Transform const *target_t = ecs_component_from_ptr(
            &engine_context->world, target, COMPONENT_ID(Transform));

    if (!target_t) {
        return;
    }

    // uint32_t *texture1 = rman_get(
    //         &engine_context->rman, RESOURCE_ID(axis_arrow_texture1));
    // uint32_t *texture2 = rman_get(
    //         &engine_context->rman, RESOURCE_ID(axis_arrow_texture2));
    // uint32_t *texture3 = rman_get(
    //         &engine_context->rman, RESOURCE_ID(axis_arrow_texture3));

    spawn_axis_arrow(
            engine_context, target_t, target, engine_context->camera.up, 0);
    // spawn_axis_arrow(engine_context, target_t, target, 0);
    // spawn_axis_arrow(engine_context, target_t, target, 0);
}

static void log_thing(EngineContext *engine_context, EntityPtr eptr) {
    spawn_axis_arrows(engine_context, eptr);
}

void test_stuff(EngineContext *engine_context) {
    vector(Model) models;
    vector_init(models);

    REGISTER_COMPONENT(&engine_context->world, AxisArrow);

    VfsFile file;
    vfs_open("test.obj", VFS_OPEN_MODE_READ, &file);
    Reader r = vfs_file_reader(&file);

    obj_model_parse(&r, &models);

    Renderable rend;
    vector_init(rend.commands);

    backend_register_mesh(&engine_context->render_context, models[0]);

    Image texture;
    load_image_file("utc16.tga", &texture);

    Rect bounds[] = {{0, 0, texture.w, texture.h}};

    uint32_t first_id;
    backend_register_texture_atlas(&engine_context->render_context,
            &texture,
            bounds,
            1,
            &first_id);

    Transform transform = {
            .bounding_box =
                    {
                            .min = {0.0, 0.0, 0.0},
                            .max = {1.0, 1.0, 1.0},
                    },
            .position = {0.0, 0.0, -1.0},
            .scale = 1.0,
            .rotation = {},
    };

    aabb_translate(&transform.bounding_box, transform.position);

    Command mesh_cmd = {.type = COMMAND_MESH,
            .mesh = {
                    .mesh_id = 0, .transform = {}, .texture_id = first_id}};
    calculate_model_matrix(&transform, mesh_cmd.mesh.transform);

    vector_append(rend.commands, mesh_cmd);

    Clickable clickable = {.clicked_callback = log_thing};

    EntityBuilder builder;
    entity_builder_init(&builder, &engine_context->world);
    entity_builder_add(&builder, COMPONENT_ID(Renderable), &rend);
    entity_builder_add(&builder, COMPONENT_ID(Transform), &transform);
    entity_builder_add(&builder, COMPONENT_ID(Clickable), &clickable);
    entity_builder_finish(&builder);
}

int plugin_load(EngineContext *engine_context) {
    REGISTER_COMPONENT(&engine_context->world, Clickable);

    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_KEY_DOWN,
            (EventHandler){.handler_fn = player_controller_handler});

    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            (EventHandler){.handler_fn = object_click_handler});

    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            (EventHandler){.handler_fn = thing2});
    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_MOUSE_MOVE,
            (EventHandler){.handler_fn = player_controller_handler_mouse});

    test_stuff(engine_context);

    return 0;
}
