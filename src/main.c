#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/mat4.h>
#include <log.h>
#include <sys/mman.h>

#include "sunset/backend.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/events.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/images.h"
#include "sunset/math.h"
#include "sunset/physics.h"
#include "sunset/render.h"
#include "sunset/scene.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

void context_init(struct context *context,
        struct font *fonts,
        size_t num_fonts,
        void *render_context,
        struct event_queue *event_queue,
        struct scene *scene) {
    context->fonts = fonts;
    context->num_fonts = num_fonts;
    context->render_context = render_context;
    command_buffer_init(&context->command_buffer, COMMAND_BUFFER_DEFAULT);
    context->event_queue = event_queue;
    context->scene = scene;

    context->mouse.first_mouse = true;
}

void context_destroy(struct context *context) {
    command_buffer_destroy(&context->command_buffer);
}

struct mesh create_test_mesh() {
    struct mesh test_mesh;

    test_mesh.num_vertices = 3;
    test_mesh.vertices = malloc(test_mesh.num_vertices * 5 * sizeof(float));

    // clang-format off
    float vertices[] = {
        // positions         // texture coords
        0.0f,  -0.5f, 0.0f,   1.0f, 1.0f, // top right
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
        0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
    };
    // clang-format on

    memcpy(test_mesh.vertices, vertices, sizeof(vertices));

    test_mesh.num_indices = 3;
    test_mesh.indices =
            (uint32_t *)malloc(test_mesh.num_indices * sizeof(uint32_t));
    test_mesh.indices[0] = 0;
    test_mesh.indices[1] = 1;
    test_mesh.indices[2] = 2;

    return test_mesh;
}

void create_test_ground_mesh(struct mesh *mesh_out) {
    mesh_out->num_vertices = 4;
    mesh_out->vertices = malloc(mesh_out->num_vertices * 5 * sizeof(float));

    // clang-format off
    float vertices[] = {
        // positions         // texture coords
         50.0f, -0.5f,  50.0f,   1.0f, 1.0f, // top right
         50.0f, -0.5f, -50.0f,   1.0f, 0.0f, // bottom right
        -50.0f, -0.5f, -50.0f,   0.0f, 0.0f, // bottom left
        -50.0f, -0.5f,  50.0f,   0.0f, 1.0f, // top left 
    };
    // clang-format on

    memcpy(mesh_out->vertices, vertices, sizeof(vertices));

    mesh_out->num_indices = 6;
    mesh_out->indices =
            (uint32_t *)malloc(mesh_out->num_indices * sizeof(uint32_t));
    // first triangle
    mesh_out->indices[0] = 0;
    mesh_out->indices[1] = 1;
    mesh_out->indices[2] = 3;
    // second triangle
    mesh_out->indices[3] = 1;
    mesh_out->indices[4] = 2;
    mesh_out->indices[5] = 3;
}

struct camera_object {
    size_t camera_idx;
    struct scene *scene;
};

void camera_move_callback(struct object *object, vec3 direction) {
    struct camera_object *camera_object = (struct camera_object *)object->data;

    scene_move_camera(
            camera_object->scene, camera_object->camera_idx, direction);
}

// should be in engine
void mouse_event_handler(struct context *context, struct event event) {
    struct point mouse_move = event.data.mouse_move;
    scene_rotate_camera(context->scene, 0, mouse_move.x, mouse_move.y);
}

int main() {
    int retval = 0;

    struct scene scene;
    struct camera camera;

    struct render_config render_config = {
            .window_width = 1920,
            .window_height = 1080,
    };

    // if ((retval = build_render_config(&render_config))) {
    //     return retval;
    // }

    struct render_context render_context;
    if (backend_setup(&render_context, render_config)) {
        log_debug("wtf");
        return -1;
    }

    struct image image;
    if ((retval = load_image_file("./utc16.tga", &image))) {
        return retval;
    }

    struct rect bounds = {
            .x = 0,
            .y = 0,
            .width = image.w,
            .height = image.h,
    };

    uint32_t texture_id;
    backend_register_texture_atlas(
            &render_context, &image, &bounds, 1, &texture_id);

    image_destroy(&image);

    camera_init(
            (struct camera_state){
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    0.0f,
                    0.0f,

            },
            (struct camera_options){.fov = glm_rad(45.0f),
                    .sensitivity = 0.01f,
                    .speed = 1.0f,
                    .aspect_ratio = (float)render_config.window_height
                            / (float)render_config.window_width},
            &camera);

    uint32_t triangle_mesh_id =
            backend_register_mesh(&render_context, create_test_mesh());

    struct mesh ground_mesh;
    create_test_ground_mesh(&ground_mesh);
    uint32_t ground_mesh_id =
            backend_register_mesh(&render_context, ground_mesh);

    struct object object = {
            .physics =
                    (struct physics_object){
                            .velocity = {1.5f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                            .damping = 0.0f,
                            .type = PHYSICS_OBJECT_REGULAR,
                            .material = {.restitution = 0.9},
                    },
            .bounding_box =
                    (struct aabb){
                            {-0.5f, 0.0f, 0.0f},
                            {0.5f, 1.0f, 0.01f},
                    },
            .transform =
                    (struct transform){
                            .position = {10.0f, 0.0f, 0.0f},
                            .rotation = {0.0f, 0.0f, 0.0f},
                            .scale = 1.0f,
                    },
            .mesh_id = triangle_mesh_id,
            .texture_id = texture_id,
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
            .label = "triangle1",
    };

    aabb_translate(&object.bounding_box, object.transform.position);

    struct object player = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                            .damping = 0.0f,
                            .type = PHYSICS_OBJECT_REGULAR,
                            .material = {.restitution = 0.9},
                    },
            .bounding_box =
                    (struct aabb){
                            {-0.5f, 0.0f, 0.0f},
                            {0.5f, 1.0f, 0.01f},
                    },
            .transform =
                    (struct transform){
                            .position = {0.0f, 0.0f, 0.0f},
                            .rotation = {0.0f, 0.0f, 0.0f},
                            .scale = 1.0f,
                    },
            .mesh_id = triangle_mesh_id,
            .texture_id = texture_id,

            .materials = NULL,
            .num_materials = 0,
            .controller = {},
            .parent = NULL,
            .children = NULL,
            .num_children = 0,
            .label = "player",
    };

    struct camera_object camera_object_data = {
            .camera_idx = 0,
            .scene = &scene,
    };

    struct object camera_object = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                            .damping = 0.0f,
                            .type = PHYSICS_OBJECT_INFINITE,
                            .material = {.restitution = 0.9},
                    },
            .bounding_box =
                    (struct aabb){
                            {-0.5f, 0.0f, 0.0f},
                            {0.5f, 1.0f, 0.01f},
                    },
            .transform =
                    (struct transform){
                            .position = {0.0f, 0.0f, 0.0f},
                            .rotation = {0.0f, 0.0f, 0.0f},
                            .scale = 1.0f,
                    },
            .mesh_id = triangle_mesh_id,
            .texture_id = texture_id,

            .materials = NULL,
            .num_materials = 0,
            .controller = {},
            .parent = &player,
            .children = NULL,
            .num_children = 0,
            .data = &camera_object_data,
            .move_callback = camera_move_callback,
            .label = "camera",
    };

    aabb_translate(&player.bounding_box, player.transform.position);
    aabb_translate(&player.bounding_box, camera_object.transform.position);

    player.children = malloc(sizeof(struct object *));
    player.children[0] = &camera_object;
    player.num_children = 1;

    struct object object2 = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                            .damping = 0.0f,
                            .type = PHYSICS_OBJECT_REGULAR,
                            .material = {.restitution = 0.9},
                    },
            .bounding_box =
                    (struct aabb){
                            {-0.5f, 0.0f, 0.0f},
                            {0.5f, 1.0f, 0.01f},
                    },
            .transform =
                    (struct transform){
                            .position = {0.0f, 0.0f, -4.0f},
                            .rotation = {0.0f, 0.0f, 0.0f},
                            .scale = 1.0f,
                    },
            .mesh_id = triangle_mesh_id,
            .texture_id = texture_id,
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
            .label = "triangle2",
    };

    aabb_translate(&object2.bounding_box, object2.transform.position);

    struct object ground_object = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = INFINITY,
                            .damping = 0.0f,
                            .type = PHYSICS_OBJECT_INFINITE,
                            .material = {.restitution = 0.9},
                    },
            .bounding_box =
                    (struct aabb){
                            {-100.0f, 0.0, -100.0f},
                            {100.0f, 0.0, 100.0f},
                    },
            .transform =
                    (struct transform){
                            .position = {0.0f, -10.0f, 0.0f},
                            .rotation = {0.0f, 0.0f, 0.0f},
                            .scale = 1.0f,
                    },
            .mesh_id = ground_mesh_id,
            .texture_id = texture_id,
            .materials = NULL,
            .num_materials = 0,
            .controller =
                    (struct controller){
                            .type = CONTROLLER_NONE,
                            .player = {},
                    },
            .parent = NULL,
            .children = NULL,
            .num_children = 0,
            .label = "ground",
    };

    aabb_translate(
            &ground_object.bounding_box, ground_object.transform.position);

    struct object **objects = malloc(sizeof(struct object *) * 3);

    // should be owned by the scene. but for now, we'll just leak it, cuz ...
    // lazyness
    objects[0] = &object;
    objects[1] = &object2;
    objects[2] = &ground_object;

    struct image skybox = {};

    struct chunk *root_chunk = malloc(sizeof(struct chunk));

    *root_chunk = (struct chunk){
            .bounds =
                    {
                            {0.0f, 0.0f, 0.0f},
                            {100.0f, 100.0f, 100.0f},
                    },
            .objects = objects,
            .num_objects = 3,
            .lights = NULL,
            .num_lights = 0,
            .id = 0,
    };

    scene_init(&camera, 1, skybox, root_chunk->bounds, root_chunk, &scene);

    struct physics physics;
    physics_init(&physics);

    physics_add_object(&physics, &object);
    physics_add_object(&physics, &object2);
    physics_add_object(&physics, &ground_object);
    physics_add_object(&physics, &player);
    physics_add_constraint(&physics, &player, &camera_object, 10.0f);

    struct event_queue event_queue;
    event_queue_init(&event_queue);

    command_buffer_init(&render_context.command_buffer, COMMAND_BUFFER_DEFAULT);

    struct font font;
    load_font_psf2("font.psf", &font);

    uint64_t avg_frame_time = 0;

    vector(uint64_t) frame_time_window;
    vector_init(frame_time_window);
    vector_reserve(frame_time_window, 100);

    struct context context;
    context_init(&context, &font, 1, &render_context, &event_queue, &scene);

    backend_set_user_context(&render_context, &context);

    event_queue_add_handler(
            &event_queue, SYSTEM_EVENT_MOUSE, mouse_event_handler);

    char fps_text_buffer[256];

    while (!backend_should_stop(&render_context)) {
        struct timespec frame_start = get_time();

        if (glfwGetKey(render_context.window, GLFW_KEY_W) == GLFW_PRESS) {
            vec3 direction = {0.0, 0.0, -1.0};
            // TODO: find a way to abstract this away (controllers?)
            camera_vec_to_world(&scene.cameras[0], direction);
            physics_move_object(&scene, &player, direction, &event_queue);
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_S) == GLFW_PRESS) {
            vec3 direction = {0.0, 0.0, 1.0};
            camera_vec_to_world(&scene.cameras[0], direction);
            physics_move_object(&scene, &player, direction, &event_queue);
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_A) == GLFW_PRESS) {
            vec3 direction = {-1.0, 0.0, 0.0};
            camera_vec_to_world(&scene.cameras[0], direction);
            physics_move_object(&scene, &player, direction, &event_queue);
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_D) == GLFW_PRESS) {
            vec3 direction = {1.0, 0.0, 0.0};
            camera_vec_to_world(&scene.cameras[0], direction);
            physics_move_object(&scene, &player, direction, &event_queue);
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(render_context.window, GLFW_TRUE);
        }

        physics_step(&physics, &scene, &event_queue, 1.0f / 60.0f);

        event_queue_process(&context, &event_queue);

        snprintf(fps_text_buffer,
                sizeof(fps_text_buffer),
                "frame time: %" PRIu64 "ms (fps: %.1f)",
                avg_frame_time / 1000,
                1000000.0f / avg_frame_time);

        command_buffer_add_set_zindex(&render_context.command_buffer, 1);
        command_buffer_add_text(&render_context.command_buffer,
                (struct point){0, 50},
                &font,
                fps_text_buffer,
                strlen(fps_text_buffer),
                WINDOW_POINT_TOP_LEFT);
        command_buffer_add_set_zindex(&render_context.command_buffer, 0);

        scene_render(&scene, &render_context);

        // TODO:
        // command_buffer_add_static_text(&command_buffer, (struct
        // point){200, 500}, &font, "hello world");
        // command_buffer_add_static_overlay command

        assert(command_buffer_empty(&render_context.command_buffer));

        uint64_t frame_time_ms = time_since_ms(frame_start);

        if (frame_time_ms < 16) {
            usleep(16 - frame_time_ms);
        }

        avg_frame_time = (avg_frame_time + time_since_us(frame_start)) / 2;

        vector_append(frame_time_window, frame_time_ms);

        if (vector_size(frame_time_window) == 100) {
            uint64_t top_1_percentile = top_percentile(frame_time_window,
                    vector_size(frame_time_window),
                    1,
                    compare_uint64_t);

            log_trace("top 1%% avg %llums", top_1_percentile);
            vector_clear(frame_time_window);
        }
    }

    // cleanup:
    return retval;
}
