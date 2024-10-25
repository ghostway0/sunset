#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <string.h>

#include <log.h>
#include <sys/mman.h>
#include <unistd.h>

#include "sunset/backend.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/physics.h"
#include "sunset/render.h"
#include "sunset/scene.h"
#include "sunset/utils.h"

void context_init(struct context *context,
        struct command_buffer_options command_buffer_options,
        struct font *fonts,
        size_t num_fonts,
        void *render_context) {
    context->fonts = fonts;
    context->num_fonts = num_fonts;
    context->render_context = render_context;
    command_buffer_init(&context->command_buffer, command_buffer_options);
    memset(context->custom_commands, 0, sizeof(context->custom_commands));
}

void context_free(struct context *context) {
    command_buffer_free(&context->command_buffer);
}

int stub_render_command(
        struct context *context, struct command const *command) {
    unused(context);

    switch (command->type) {
        case COMMAND_NOP:
            log_info("command_nop");
            break;
        case COMMAND_LINE: {
            struct command_line const *line = &command->data.line;
            log_info("command_line: %d %d %d %d",
                    line->from.x,
                    line->from.y,
                    line->to.x,
                    line->to.y);
            break;
        }
        case COMMAND_RECT: {
            struct command_rect const *rect = &command->data.rect;
            log_info("command_rect: %d %d %d %d",
                    rect->rect.x,
                    rect->rect.y,
                    rect->rect.width,
                    rect->rect.height);
            break;
        }
        case COMMAND_FILLED_RECT: {
            struct command_filled_rect const *filled_rect =
                    &command->data.filled_rect;
            log_info("command_filled_rect: %d %d %d %d",
                    filled_rect->rect.x,
                    filled_rect->rect.y,
                    filled_rect->rect.width,
                    filled_rect->rect.height);
            break;
        }
        case COMMAND_ARC: {
            struct command_arc const *arc = &command->data.arc;
            log_info("command_arc: %d %d %d %f %f",
                    arc->center.x,
                    arc->center.y,
                    arc->r,
                    arc->a0,
                    arc->a1);
            break;
        }
        case COMMAND_FILLED_ARC: {
            struct command_filled_arc const *filled_arc =
                    &command->data.filled_arc;
            log_info("command_filled_arc: %d %d %d %f %f",
                    filled_arc->center.x,
                    filled_arc->center.y,
                    filled_arc->r,
                    filled_arc->a0,
                    filled_arc->a1);
            break;
        }
        case COMMAND_TEXT: {
            struct command_text const *text = &command->data.text;
            log_info("command_text: %d %d %s",
                    text->start.x,
                    text->start.y,
                    text->text);
            break;
        }
        case COMMAND_IMAGE: {
            struct command_image const *image = &command->data.image;
            log_info("command_image: %d %d %zu %zu",
                    image->pos.x,
                    image->pos.y,
                    image->image.w,
                    image->image.h);
            show_image_grayscale_at(&image->image, image->pos);
            break;
        }
        default: {
            size_t custom_command_idx = command->type - NUM_COMMANDS;
            log_info("custom_command %zu", custom_command_idx);
        }
    }

    return 0;
}

struct mesh create_test_mesh() {
    struct mesh test_mesh;

    // Allocate memory for 3 vertices
    test_mesh.num_vertices = 3;
    test_mesh.vertices = (vec3 *)malloc(test_mesh.num_vertices * sizeof(vec3));

    // Initialize vertices for a triangle
    test_mesh.vertices[0][0] = 0.0f;
    test_mesh.vertices[0][1] = 0.5f;
    test_mesh.vertices[0][2] = 0.0f;
    test_mesh.vertices[1][0] = -0.5f;
    test_mesh.vertices[1][1] = -0.5f;
    test_mesh.vertices[1][2] = 0.0f;
    test_mesh.vertices[2][0] = 0.5f;
    test_mesh.vertices[2][1] = -0.5f;
    test_mesh.vertices[2][2] = 0.0f;

    // Allocate and initialize indices for the triangle
    test_mesh.num_indices = 3;
    test_mesh.indices =
            (uint32_t *)malloc(test_mesh.num_indices * sizeof(uint32_t));
    test_mesh.indices[0] = 0;
    test_mesh.indices[1] = 1;
    test_mesh.indices[2] = 2;

    return test_mesh;
}

int main() {
    int retval = 0;

    struct scene scene;
    struct camera camera;

    camera_init(
            (struct camera_state){
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    0.0f,
                    0.0f,

            },
            (struct camera_options){
                    0.1f,
                    100.0f,
                    45.0f,
                    0.75f,
            },
            &camera);

    struct object object = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                    },
            .bounding_box =
                    (struct box){
                            {0.0f, 0.0f, 0.0f},
                            {1.0f, 1.0f, 1.0f},
                    },
            .transform =
                    (struct transform){
                            {0.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 0.0f},
                            1.0f,
                    },
            .meshes = NULL,
            .num_meshes = 0,
            .textures = NULL,
            .num_textures = 0,
            .materials = NULL,
            .num_materials = 0,
            .controller =
                    (struct controller){
                            .type = CONTROLLER_PLAYER,
                            .player = {},
                    },
            .parent = NULL,
            .children = NULL,
            .num_children = 0,
    };

    struct object object2 = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                    },
            .bounding_box =
                    (struct box){
                            {0.0f, 0.0f, 0.0f},
                            {1.0f, 1.0f, 1.0f},
                    },
            .transform =
                    (struct transform){
                            {0.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 0.0f},
                            1.0f,
                    },
            .meshes = NULL,
            .num_meshes = 0,
            .textures = NULL,
            .num_textures = 0,
            .materials = NULL,
            .num_materials = 0,
            .controller =
                    (struct controller){
                            .type = CONTROLLER_PLAYER,
                            .player = {},
                    },
            .parent = NULL,
            .children = NULL,
            .num_children = 0,
    };

    struct object **objects = malloc(sizeof(struct object *) * 2);

    // should be owned by the scene. but for now, we'll just leak it, cuz ...
    // lazyness
    objects[0] = &object;
    objects[1] = &object2;

    struct image skybox = {};

    struct chunk *root_chunk = malloc(sizeof(struct chunk));

    *root_chunk = (struct chunk){
            .bounds = {0},
            .objects = objects,
            .num_objects = 2,
            .lights = NULL,
            .num_lights = 0,
            .id = 0,
    };

    scene_init(camera, skybox, NULL, 0, root_chunk->bounds, root_chunk, &scene);

    struct physics physics;
    physics_init(&physics);

    physics_add_object(&physics, &object);
    physics_add_object(&physics, &object2);
    physics_add_constraint(&physics, &object, &object2, 2.0f);

    struct event_queue event_queue;
    event_queue_init(&event_queue);

    physics_step(&physics, &scene, &event_queue, 1 / 60.0f);

    for (size_t i = 0; i < vector_size(event_queue.events); i++) {
        struct event event = event_queue.events[i];

        switch (event.type_id) {
            case SYSTEM_EVENT_COLLISION: {
                struct collision_event *collision_event =
                        (struct collision_event *)event.data;
                log_debug("collision: %p %p",
                        collision_event->a,
                        collision_event->b);
                break;
            }
            default:
                log_debug("event %u", event.type_id);
        }
    }

    struct render_context render_context = {};

    if (backend_setup(&render_context,
                (struct render_config){.width = 800, .height = 600})) {
        log_debug("wtf");
        return -1;
    }

    struct mesh test_mesh = create_test_mesh();

    struct compiled_mesh compiled;

    compile_mesh(&test_mesh, &compiled);

    while (!glfwWindowShouldClose(render_context.window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        backend_draw_mesh(&compiled);

        glfwSwapBuffers(render_context.window);
        glfwPollEvents();
    }

    event_queue_free(&event_queue);
    scene_destroy(&scene);
    physics_free(&physics);

    // cleanup:
    return retval;
}
