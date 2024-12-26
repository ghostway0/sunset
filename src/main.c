#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <cglm/mat4.h>
#include <log.h>

#include "sunset/engine.h"

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

int main() {
    int retval = 0;

    // struct render_config render_config = {
    //         .window_width = 1920,
    //         .window_height = 1080,
    // };

    // engine_run();

    // uint32_t texture_id;
    // backend_register_texture_atlas(
    //         &render_context, &image, &bounds, 1, &texture_id);
    //
    // image_destroy(&image);
    //
    // uint32_t triangle_mesh_id =
    //         backend_register_mesh(&render_context, create_test_mesh());
    //
    // struct mesh ground_mesh;
    // create_test_ground_mesh(&ground_mesh);
    // uint32_t ground_mesh_id =
    //         backend_register_mesh(&render_context, ground_mesh);
    //
    // struct object object = {
    //         .physics =
    //                 (struct physics_object){
    //                         .velocity = {1.5f, 0.0f, 0.0f},
    //                         .acceleration = {0.0f, 0.0f, 0.0f},
    //                         .mass = 1.0f,
    //                         .damping = 0.0f,
    //                         .type = PHYSICS_OBJECT_REGULAR,
    //                         .material = {.restitution = 0.9},
    //                 },
    //         .bounding_box =
    //                 (struct aabb){
    //                         {-0.5f, 0.0f, 0.0f},
    //                         {0.5f, 1.0f, 0.01f},
    //                 },
    //         .transform =
    //                 (struct transform){
    //                         .position = {10.0f, 0.0f, 0.0f},
    //                         .rotation = {0.0f, 0.0f, 0.0f},
    //                         .scale = 1.0f,
    //                 },
    //         .mesh_id = triangle_mesh_id,
    //         .texture_id = texture_id,
    //         .materials = NULL,
    //         .num_materials = 0,
    //         .controller =
    //                 (struct controller){
    //                         .type = CONTROLLER_PLAYER,
    //                         .player = {},
    //                 },
    //         .parent = NULL,
    //         .children = NULL,
    //         .num_children = 0,
    //         .label = "triangle1",
    // };
    //
    // aabb_translate(&object.bounding_box, object.transform.position);
    //
    // struct object player = {
    //         .physics =
    //                 (struct physics_object){
    //                         .velocity = {0.0f, 0.0f, 0.0f},
    //                         .acceleration = {0.0f, 0.0f, 0.0f},
    //                         .mass = 1.0f,
    //                         .damping = 0.0f,
    //                         .type = PHYSICS_OBJECT_REGULAR,
    //                         .material = {.restitution = 0.9},
    //                 },
    //         .bounding_box =
    //                 (struct aabb){
    //                         {-0.5f, 0.0f, 0.0f},
    //                         {0.5f, 1.0f, 0.01f},
    //                 },
    //         .transform =
    //                 (struct transform){
    //                         .position = {0.0f, 0.0f, 0.0f},
    //                         .rotation = {0.0f, 0.0f, 0.0f},
    //                         .scale = 1.0f,
    //                 },
    //         .mesh_id = triangle_mesh_id,
    //         .texture_id = texture_id,
    //
    //         .materials = NULL,
    //         .num_materials = 0,
    //         .controller = {},
    //         .parent = NULL,
    //         .children = NULL,
    //         .num_children = 0,
    //         .label = "player",
    // };
    //
    // struct camera_object camera_object_data = {
    //         .camera_idx = 0,
    //         .scene = &scene,
    // };
    //
    // struct object camera_object = {
    //         .physics =
    //                 (struct physics_object){
    //                         .velocity = {0.0f, 0.0f, 0.0f},
    //                         .acceleration = {0.0f, 0.0f, 0.0f},
    //                         .mass = 1.0f,
    //                         .damping = 0.0f,
    //                         .type = PHYSICS_OBJECT_INFINITE,
    //                         .material = {.restitution = 0.9},
    //                 },
    //         .bounding_box =
    //                 (struct aabb){
    //                         {-0.5f, 0.0f, 0.0f},
    //                         {0.5f, 1.0f, 0.01f},
    //                 },
    //         .transform =
    //                 (struct transform){
    //                         .position = {0.0f, 0.0f, 0.0f},
    //                         .rotation = {0.0f, 0.0f, 0.0f},
    //                         .scale = 1.0f,
    //                 },
    //         .mesh_id = triangle_mesh_id,
    //         .texture_id = texture_id,
    //
    //         .materials = NULL,
    //         .num_materials = 0,
    //         .controller = {},
    //         .parent = &player,
    //         .children = NULL,
    //         .num_children = 0,
    //         .data = &camera_object_data,
    //         .move_callback = camera_move_callback,
    //         .label = "camera",
    // };
    //
    // aabb_translate(&player.bounding_box, player.transform.position);
    // aabb_translate(&player.bounding_box, camera_object.transform.position);
    //
    // player.children = malloc(sizeof(struct object *));
    // player.children[0] = &camera_object;
    // player.num_children = 1;
    //
    // struct object object2 = {
    //         .physics =
    //                 (struct physics_object){
    //                         .velocity = {0.0f, 0.0f, 0.0f},
    //                         .acceleration = {0.0f, 0.0f, 0.0f},
    //                         .mass = 1.0f,
    //                         .damping = 0.0f,
    //                         .type = PHYSICS_OBJECT_REGULAR,
    //                         .material = {.restitution = 0.9},
    //                 },
    //         .bounding_box =
    //                 (struct aabb){
    //                         {-0.5f, 0.0f, 0.0f},
    //                         {0.5f, 1.0f, 0.01f},
    //                 },
    //         .transform =
    //                 (struct transform){
    //                         .position = {0.0f, 0.0f, -4.0f},
    //                         .rotation = {0.0f, 0.0f, 0.0f},
    //                         .scale = 1.0f,
    //                 },
    //         .mesh_id = triangle_mesh_id,
    //         .texture_id = texture_id,
    //         .materials = NULL,
    //         .num_materials = 0,
    //         .controller =
    //                 (struct controller){
    //                         .type = CONTROLLER_PLAYER,
    //                         .player = {},
    //                 },
    //         .parent = NULL,
    //         .children = NULL,
    //         .num_children = 0,
    //         .label = "triangle2",
    // };
    //
    // aabb_translate(&object2.bounding_box, object2.transform.position);
    //
    // struct object ground_object = {
    //         .physics =
    //                 (struct physics_object){
    //                         .velocity = {0.0f, 0.0f, 0.0f},
    //                         .acceleration = {0.0f, 0.0f, 0.0f},
    //                         .mass = INFINITY,
    //                         .damping = 0.0f,
    //                         .type = PHYSICS_OBJECT_INFINITE,
    //                         .material = {.restitution = 0.9},
    //                 },
    //         .bounding_box =
    //                 (struct aabb){
    //                         {-100.0f, 0.0, -100.0f},
    //                         {100.0f, 0.0, 100.0f},
    //                 },
    //         .transform =
    //                 (struct transform){
    //                         .position = {0.0f, -10.0f, 0.0f},
    //                         .rotation = {0.0f, 0.0f, 0.0f},
    //                         .scale = 1.0f,
    //                 },
    //         .mesh_id = ground_mesh_id,
    //         .texture_id = texture_id,
    //         .materials = NULL,
    //         .num_materials = 0,
    //         .controller =
    //                 (struct controller){
    //                         .type = CONTROLLER_NONE,
    //                         .player = {},
    //                 },
    //         .parent = NULL,
    //         .children = NULL,
    //         .num_children = 0,
    //         .label = "ground",
    // };
    //
    // aabb_translate(
    //         &ground_object.bounding_box, ground_object.transform.position);

    // cleanup:
    return retval;
}
