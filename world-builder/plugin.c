#include <stdint.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>
#include <log.h>

#include "internal/math.h"
#include "internal/utils.h"
#include "map.h"
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

DECLARE_RESOURCE_ID(spawned_arrows);

typedef struct Clickable {
    void (*clicked_callback)(EngineContext *, EntityPtr clicked_entity);
    void (*dragged_callback)(
            EngineContext *, EntityPtr clicked_entity, Point offset);
    uint64_t click_id;
} Clickable;

DECLARE_COMPONENT_ID(Clickable);

static void object_drag_handler(EngineContext *engine_context,
        void * /*local_context*/,
        Event const event) {
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (*focus != FOCUS_MAIN) {
        return;
    }

    MouseMoveEvent *mouse_move = (MouseMoveEvent *)event.data;

    if (!bitmask_is_set(&mouse_move->mouse_buttons, MOUSE_BUTTON_LEFT)) {
        return;
    }

    Bitmask bitmask;
    bitmask_init_empty(ECS_MAX_COMPONENTS, &bitmask);
    bitmask_set(&bitmask, COMPONENT_ID(Clickable));

    WorldIterator it = worldit_create(&engine_context->world, bitmask);
    while (worldit_is_valid(&it)) {
        EntityPtr eptr = worldit_get_entityptr(&it);

        Clickable *clickable = ecs_component_from_ptr(
                &engine_context->world, eptr, COMPONENT_ID(Clickable));

        if (clickable->dragged_callback
                && clickable->click_id + 1
                        == backend_get_click_id(
                                &engine_context->render_context)) {
            clickable->dragged_callback(
                    engine_context, eptr, mouse_move->offset);
        }

        worldit_advance(&it);
    }

    worldit_destroy(&it);
}

static void object_click_handler(EngineContext *engine_context,
        void * /*local_context*/,
        Event const event) {
    uint32_t *focus =
            rman_get(&engine_context->rman, RESOURCE_ID(input_focus));

    if (*focus != FOCUS_MAIN) {
        return;
    }

    MouseClickEvent *mouse_click = (MouseClickEvent *)event.data;

    if (mouse_click->button != MOUSE_BUTTON_LEFT) {
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
                    &engine_context->camera, &transform->bounding_box)) {
            clickable->click_id = mouse_click->click_id;

            if (clickable->clicked_callback) {
                clickable->clicked_callback(engine_context, eptr);
            }
        }

        worldit_advance(&it);
    }

    worldit_destroy(&it);
}

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
    vec3 axis;
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

static void axis_arrow_dragged(
        EngineContext *engine_context, EntityPtr eptr, Point offset) {
    AxisArrow *axis_arrow = ecs_component_from_ptr(
            &engine_context->world, eptr, COMPONENT_ID(AxisArrow));
    assert(axis_arrow);

    Transform *parent_t = ecs_component_from_ptr(&engine_context->world,
            axis_arrow->parent,
            COMPONENT_ID(Transform));

    float off = (-offset.x + offset.y) * 0.01;

    vec3 offset_vec;
    glm_vec3_scale(axis_arrow->axis, off, offset_vec);

    glm_vec3_add(parent_t->position, offset_vec, parent_t->position);
    aabb_translate(&parent_t->bounding_box, offset_vec);
}

void spawn_axis_arrow(EngineContext *engine_context,
        Transform const *transform,
        EntityPtr parent,
        vec3 axis, // should be const
        uint32_t texture) {
    Renderable rend;
    vector_init(rend.commands);

    uint32_t *mesh_id =
            rman_get(&engine_context->rman, RESOURCE_ID(axis_arrow_mesh));

    Command mesh_cmd = {.type = COMMAND_MESH,
            .mesh = {.mesh_id = *mesh_id,
                    .texture_id = texture}};

    Transform new_transform = {};

    new_transform.bounding_box =
            (AABB){.min = {0.0, 0.0, 0.0}, .max = {0.1, 0.1, 0.2}};
    vec3 pos;
    aabb_get_face_center(&transform->bounding_box, axis, pos);
    aabb_translate(&new_transform.bounding_box, pos);
    glm_vec3_copy(pos, new_transform.position);

    new_transform.scale = 0.1f;

    vec3 default_forward = {0.0f, 0.0f, 1.0f};
    vec3 rotation_axis;
    glm_vec3_cross(default_forward, axis, rotation_axis);

    float dot = glm_vec3_dot(default_forward, axis);

    if (glm_vec3_norm2(rotation_axis) > EPSILON) {
        glm_vec3_normalize(rotation_axis);
        glm_vec3_scale(rotation_axis, acos(dot), new_transform.rotation);
    } else {
        glm_vec3_zero(new_transform.rotation);
    }

    vector_append(rend.commands, mesh_cmd);

    AxisArrow arrow = {.parent = parent};
    glm_vec3_copy(axis, arrow.axis);

    Clickable clickable = {.dragged_callback = axis_arrow_dragged};

    EntityBuilder builder;
    entity_builder_init(&builder, &engine_context->world);
    entity_builder_add(&builder, COMPONENT_ID(Transform), &new_transform);
    entity_builder_add(&builder, COMPONENT_ID(Renderable), &rend);
    entity_builder_add(&builder, COMPONENT_ID(AxisArrow), &arrow);
    entity_builder_add(&builder, COMPONENT_ID(Clickable), &clickable);
    entity_builder_finish(&builder);
}

static Order order_entityptr(const void *a, const void *b) {
    EntityPtr *eptr_a = (EntityPtr *)a;
    EntityPtr *eptr_b = (EntityPtr *)b;

    if (eptr_a->archetype < eptr_b->archetype) {
        return ORDER_LESS_THAN;
    }
    
    if (eptr_a->archetype > eptr_b->archetype) {
        return ORDER_GREATER_THAN;
    }

    if (eptr_a->element < eptr_b->element) {
        return ORDER_LESS_THAN;
    }
    
    if (eptr_a->element > eptr_b->element) {
        return ORDER_GREATER_THAN;
    }

    return ORDER_EQUAL;
}

void spawn_axis_arrows(EngineContext *engine_context, EntityPtr target) {
    Transform const *target_t = ecs_component_from_ptr(
            &engine_context->world, target, COMPONENT_ID(Transform));

    if (!target_t) {
        return;
    }

    map(EntityPtr) *spawned_arrows = rman_get(
            &engine_context->rman, RESOURCE_ID(spawned_arrows));

    if (map_get(*spawned_arrows, target, order_entityptr)) {
        return;
    }

    uint32_t *texture1 = rman_get(
            &engine_context->rman, RESOURCE_ID(axis_arrow_texture1));
    uint32_t *texture2 = rman_get(
            &engine_context->rman, RESOURCE_ID(axis_arrow_texture2));
    uint32_t *texture3 = rman_get(
            &engine_context->rman, RESOURCE_ID(axis_arrow_texture3));

    spawn_axis_arrow(
            engine_context, target_t, target, (vec3){0, 1, 0}, *texture1);
    spawn_axis_arrow(
            engine_context, target_t, target, (vec3){1, 0, 0}, *texture2);
    spawn_axis_arrow(
            engine_context, target_t, target, (vec3){0, 0, -1}, *texture3);

    map_insert(*spawned_arrows, target, order_entityptr);
}

static void subject_click(EngineContext *engine_context, EntityPtr eptr) {
    spawn_axis_arrows(engine_context, eptr);
}

static void crosshair_handler(
        EngineContext *engine_context, void *command, Event const event) {
    unused(engine_context);
    unused(command);
    unused(event);
    Command *image_cmd = command;
    unused(image_cmd);
    // image_cmd->image.pos = .{};
}

void crosshair(EngineContext *engine_context) {
    Renderable rend;
    vector_init(rend.commands);

    Image crosshair;
    if (load_image_file("./engine.tga", &crosshair)) {
        log_error("wtf");
    }

    // TODO: coordinates should probably be screen coordinates [-1, 1]
    Command image_cmd = {
            .type = COMMAND_IMAGE,
            .image = {.pos = {engine_context->render_context.screen_width
                                      / 2,
                              engine_context->render_context.screen_height
                                      / 2},
                    .image = crosshair,
                    .bounds = {0, 0, 100, 100}},
    };
    vector_append(rend.commands, image_cmd);

    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_VIEWPORT_CHANGED,
            (EventHandler){.handler_fn = crosshair_handler,
                    .local_context = rend.commands});

    EntityBuilder builder;
    entity_builder_init(&builder, &engine_context->world);
    entity_builder_add(&builder, COMPONENT_ID(Renderable), &rend);
    entity_builder_finish(&builder);
}

void test_stuff(EngineContext *engine_context) {
    vector(Model) models;
    vector_init(models);

    REGISTER_COMPONENT(&engine_context->world, AxisArrow);

    VfsFile file;
    if (vfs_open("axis_arrow.obj", VFS_OPEN_MODE_READ, &file)) {
        log_error("oh no");
    }

    Reader r = vfs_file_reader(&file);

    obj_model_parse(&r, &models);

    Renderable rend;
    vector_init(rend.commands);

    uint32_t mesh_id = backend_register_mesh(
            &engine_context->render_context, models[0]);

    Image texture;
    if (load_image_file("utc16.tga", &texture)) {
        log_error("oh no");
    }

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
                            .max = {0.5, 0.5, 0.5},
                    },
            .position = {0.0, 0.0, -1.0},
            .scale = 1.0,
            .rotation = {},
    };

    aabb_translate(&transform.bounding_box, transform.position);

    Command mesh_cmd = {.type = COMMAND_MESH,
            .mesh = {.instanced = false,
                    .mesh_id = mesh_id,
                    .texture_id = UINT32_MAX}};

    vector_append(rend.commands, mesh_cmd);

    Clickable clickable = {.clicked_callback = subject_click};

    EntityBuilder builder;
    entity_builder_init(&builder, &engine_context->world);
    entity_builder_add(&builder, COMPONENT_ID(Renderable), &rend);
    entity_builder_add(&builder, COMPONENT_ID(Transform), &transform);
    entity_builder_add(&builder, COMPONENT_ID(Clickable), &clickable);
    entity_builder_finish(&builder);

    if (load_image_file("axis_arrows.tga", &texture)) {
        log_error("oh no");
    }

    Rect bounds2[] = {{0, 0, 3888, 2592},
            {0, 4790, 3000, 2000},
            {0, 2593, 3387, 2196}};

    backend_register_texture_atlas(&engine_context->render_context,
            &texture,
            bounds2,
            3,
            &first_id);

    REGISTER_RESOURCE(&engine_context->rman, axis_arrow_mesh, mesh_id);
    REGISTER_RESOURCE(&engine_context->rman, axis_arrow_texture1, first_id);
    REGISTER_RESOURCE(
            &engine_context->rman, axis_arrow_texture2, first_id + 1);
    REGISTER_RESOURCE(
            &engine_context->rman, axis_arrow_texture3, first_id + 2);
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
            SYSTEM_EVENT_MOUSE_MOVE,
            (EventHandler){.handler_fn = object_drag_handler});

    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_MOUSE_CLICK,
            (EventHandler){.handler_fn = thing2});
    event_queue_add_handler(&engine_context->event_queue,
            SYSTEM_EVENT_MOUSE_MOVE,
            (EventHandler){.handler_fn = player_controller_handler_mouse});

    test_stuff(engine_context);
    // crosshair(engine_context);

    map(EntityPtr) arrows_created;
    map_init(arrows_created);

    REGISTER_RESOURCE(&engine_context->rman, spawned_arrows, arrows_created);

    return 0;
}
