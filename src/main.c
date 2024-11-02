#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <cglm/mat4.h>
#include <log.h>
#include <sys/mman.h>

#include "cglm/util.h"
#include "sunset/backend.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
#include "sunset/events.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/physics.h"
#include "sunset/render.h"
#include "sunset/scene.h"
#include "sunset/utils.h"
#include "sunset/vector.h"

uint64_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

struct timespec get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

uint64_t time_since_ms(struct timespec start) {
    struct timespec now = get_time();
    return (now.tv_sec - start.tv_sec) * 1000
            + (now.tv_nsec - start.tv_nsec) / 1000000;
}

uint64_t time_since_us(struct timespec start) {
    struct timespec now = get_time();
    return (now.tv_sec - start.tv_sec) * 1000000
            + (now.tv_nsec - start.tv_nsec) / 1000;
}

uint64_t time_elapsed_ms(struct timespec start) {
    struct timespec now = get_time();
    return (now.tv_sec - start.tv_sec) * 1000
            + (now.tv_nsec - start.tv_nsec) / 1000000;
}

uint64_t time_elapsed_us(struct timespec start) {
    struct timespec now = get_time();
    return (now.tv_sec - start.tv_sec) * 1000000
            + (now.tv_nsec - start.tv_nsec) / 1000;
}

uint64_t get_time_us() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int compare_uint64_t(void const *a, void const *b) {
    return *(uint64_t *)a - *(uint64_t *)b;
}

#define top_percentile(arr, n, p, compare)                                     \
    ({                                                                         \
        size_t *sorted = malloc(n * sizeof(size_t));                           \
        memcpy(sorted, arr, n * sizeof(size_t));                               \
        qsort(sorted, n, sizeof(size_t), compare);                             \
        size_t idx = n - n * p / 100;                                          \
        size_t result = sorted[idx];                                           \
        free(sorted);                                                          \
        result;                                                                \
    })

void context_init(struct context *context,
        struct command_buffer_options command_buffer_options,
        struct font *fonts,
        size_t num_fonts,
        void *render_context,
        struct event_queue *event_queue) {
    context->fonts = fonts;
    context->num_fonts = num_fonts;
    context->render_context = render_context;
    command_buffer_init(&context->command_buffer, command_buffer_options);
    context->event_queue = event_queue;

    context->mouse.first_mouse = true;
}

void context_free(struct context *context) {
    command_buffer_free(&context->command_buffer);
}

struct mesh create_test_mesh() {
    struct mesh test_mesh;

    test_mesh.num_vertices = 3;
    test_mesh.vertices = (vec3 *)malloc(test_mesh.num_vertices * sizeof(vec3));

    test_mesh.vertices[0][0] = 0.0f;
    test_mesh.vertices[0][1] = 0.5f;
    test_mesh.vertices[0][2] = 0.0f;
    test_mesh.vertices[1][0] = -0.5f;
    test_mesh.vertices[1][1] = -0.5f;
    test_mesh.vertices[1][2] = 0.0f;
    test_mesh.vertices[2][0] = 0.5f;
    test_mesh.vertices[2][1] = -0.5f;
    test_mesh.vertices[2][2] = 0.0f;

    test_mesh.num_indices = 3;
    test_mesh.indices =
            (uint32_t *)malloc(test_mesh.num_indices * sizeof(uint32_t));
    test_mesh.indices[0] = 0;
    test_mesh.indices[1] = 1;
    test_mesh.indices[2] = 2;

    return test_mesh;
}

static void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    struct context *context = glfwGetWindowUserPointer(window);

    if (context->mouse.first_mouse) {
        context->mouse.x = xpos;
        context->mouse.y = ypos;
        context->mouse.first_mouse = false;
        return;
    }

    float xoffset = context->mouse.x - xpos;
    float yoffset = context->mouse.y - ypos;

    event_queue_push(context->event_queue,
            (struct event){
                    .type_id = SYSTEM_EVENT_MOUSE,
                    .data.mouse_move =
                            (struct mouse_move_event){
                                    .x = xoffset,
                                    .y = yoffset,
                            },
            });

    context->mouse.x = xpos;
    context->mouse.y = ypos;
}

struct mesh create_test_mesh2() {
    struct mesh test_mesh;

    test_mesh.num_vertices = 3;
    test_mesh.vertices = (vec3 *)malloc(test_mesh.num_vertices * sizeof(vec3));

    test_mesh.vertices[0][0] = 0.5f;
    test_mesh.vertices[0][1] = 0.5f;
    test_mesh.vertices[0][2] = 0.0f;
    test_mesh.vertices[1][0] = -0.5f;
    test_mesh.vertices[1][1] = -0.5f;
    test_mesh.vertices[1][2] = 0.0f;
    test_mesh.vertices[2][0] = 0.5f;
    test_mesh.vertices[2][1] = -0.5f;
    test_mesh.vertices[2][2] = 0.0f;

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

    struct render_context render_context = {};

    if (backend_setup(&render_context,
                (struct render_config){
                        .window_width = 1920, .window_height = 1080})) {
        log_debug("wtf");
        return -1;
    }

    camera_init(
            (struct camera_state){
                    {0.0f, 0.0f, 0.0f},
                    {0.0f, 1.0f, 0.0f},
                    0.0f,
                    0.0f,

            },
            (struct camera_options){
                    .fov = glm_rad(45.0f),
                    .sensitivity = 0.01f,
                    .speed = 0.1f,
                    .aspect_ratio = 1920.0 / 1080.0,
            },
            &camera);

    uint32_t triangle_mesh_id =
            backend_register_mesh(&render_context, create_test_mesh());

    struct object object = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, -0.3f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                            .damping = 0.0f,
                            .should_fix = true,
                    },
            .bounding_box =
                    (struct box){
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

    box_translate(&object.bounding_box, object.transform.position);

    struct object object2 = {
            .physics =
                    (struct physics_object){
                            .velocity = {0.0f, 0.0f, 0.0f},
                            .acceleration = {0.0f, 0.0f, 0.0f},
                            .mass = 1.0f,
                            .damping = 0.0f,
                            .should_fix = true,
                    },
            .bounding_box =
                    (struct box){
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

    box_translate(&object2.bounding_box, object2.transform.position);

    struct object **objects = malloc(sizeof(struct object *) * 2);

    // should be owned by the scene. but for now, we'll just leak it, cuz ...
    // lazyness
    objects[0] = &object;
    objects[1] = &object2;

    struct image skybox = {};

    struct chunk *root_chunk = malloc(sizeof(struct chunk));

    *root_chunk = (struct chunk){
            .bounds =
                    {
                            {0.0f, 0.0f, 0.0f},
                            {100.0f, 100.0f, 100.0f},
                    },
            .objects = objects,
            .num_objects = 2,
            .lights = NULL,
            .num_lights = 0,
            .id = 0,
    };

    scene_init(&camera,
            1,
            skybox,
            NULL,
            0,
            root_chunk->bounds,
            root_chunk,
            &scene);

    struct physics physics;
    physics_init(&physics);

    physics_add_object(&physics, &object);
    physics_add_object(&physics, &object2);

    struct event_queue event_queue;
    event_queue_init(&event_queue);

    command_buffer_init(&render_context.command_buffer, COMMAND_BUFFER_DEFAULT);

    struct font font;
    load_font_psf2("font.psf", "robinlinden", &font);

    uint64_t avg_frame_time = 0;

    uint64_t frame_time_window[100] = {0};
    size_t frame_time_window_idx = 0;

    glfwSwapInterval(1);

    struct context context;
    context_init(&context,
            COMMAND_BUFFER_DEFAULT,
            &font,
            1,
            &render_context,
            &event_queue);

    backend_set_user_context(&render_context, &context);

    glfwSetInputMode(render_context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(render_context.window, mouse_callback);

    while (!glfwWindowShouldClose(render_context.window)) {
        struct timespec frame_start = get_time();

        if (glfwGetKey(render_context.window, GLFW_KEY_W) == GLFW_PRESS) {
            scene_move_camera(&scene, 0, (vec3){0.0f, 0.0f, -1.0f});
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_S) == GLFW_PRESS) {
            scene_move_camera(&scene, 0, (vec3){0.0f, 0.0f, 1.0f});
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_A) == GLFW_PRESS) {
            scene_move_camera(&scene, 0, (vec3){-1.0f, 0.0f, 0.0f});
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_D) == GLFW_PRESS) {
            scene_move_camera(&scene, 0, (vec3){1.0f, 0.0f, 0.0f});
        }

        if (glfwGetKey(render_context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(render_context.window, GLFW_TRUE);
        }

        char *buffer;
        asprintf(&buffer,
                "frame time: %lums (fps: %.1f)",
                avg_frame_time / 1000,
                1000000.0f / avg_frame_time);

        physics_step(&physics, &scene, &event_queue, 1 / 60.0f);

        for (;;) {
            struct event event;
            if (event_queue_pop(&event_queue, &event)) {
                break;
            }

            switch (event.type_id) {
                case SYSTEM_EVENT_COLLISION: {
                    struct collision_event collision = event.data.collision;
                    log_debug("collision between %p and %p",
                            collision.a,
                            collision.b);
                    break;
                }
                case SYSTEM_EVENT_MOUSE: {
                    struct mouse_move_event mouse_move = event.data.mouse_move;

                    scene_rotate_camera(&scene, 0, mouse_move.x, mouse_move.y);
                    break;
                }
            }
        }

        command_buffer_add_set_zindex(&render_context.command_buffer, 1);
        command_buffer_add_text(&render_context.command_buffer,
                (struct point){0, 50},
                &font,
                buffer,
                strlen(buffer),
                WINDOW_POINT_TOP_LEFT);
        command_buffer_add_set_zindex(&render_context.command_buffer, 0);

        scene_render(&scene, &render_context);

        // TODO:
        // command_buffer_add_static_text(&command_buffer, (struct
        // point){200, 500}, &font, "hello world");
        // command_buffer_add_static_overlay command

        assert(command_buffer_empty(&render_context.command_buffer));
        free(buffer);

        uint64_t frame_time = time_elapsed_us(frame_start);
        if (frame_time < 16000) {
            usleep(16000 - frame_time);
        }

        avg_frame_time = (avg_frame_time + frame_time) / 2;

        frame_time_window[frame_time_window_idx] = frame_time;
        frame_time_window_idx = (frame_time_window_idx + 1)
                % (sizeof(frame_time_window) / sizeof(frame_time_window[0]));

        if (frame_time_window_idx == 0) {
            uint64_t top_1_percentile = top_percentile(frame_time_window,
                    sizeof(frame_time_window) / sizeof(frame_time_window[0]),
                    1,
                    compare_uint64_t);

            log_trace("top 1%% avg %llums", top_1_percentile / 1000);
        }
    }

    // cleanup:
    return retval;
}
