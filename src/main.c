#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <cglm/mat4.h>
#include <log.h>

#include "sunset/ecs.h"
#include "sunset/engine.h"
#include "sunset/render.h"
#include "vector.h"

// struct mesh create_test_mesh() {
//     struct mesh test_mesh;
//
//     test_mesh.num_vertices = 3;
//     test_mesh.vertices = malloc(test_mesh.num_vertices * 5 *
//     sizeof(float));
//
//     // clang-format off
//     float vertices[] = {
//         // positions         // texture coords
//         0.0f,  -0.5f, 0.0f,   1.0f, 1.0f, // top right
//         -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
//         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
//     };
//     // clang-format on
//
//     memcpy(test_mesh.vertices, vertices, sizeof(vertices));
//
//     test_mesh.num_indices = 3;
//     test_mesh.indices =
//             (uint32_t *)malloc(test_mesh.num_indices * sizeof(uint32_t));
//     test_mesh.indices[0] = 0;
//     test_mesh.indices[1] = 1;
//     test_mesh.indices[2] = 2;
//
//     return test_mesh;
// }
//
// void create_test_ground_mesh(struct mesh *mesh_out) {
//     mesh_out->num_vertices = 4;
//     mesh_out->vertices = malloc(mesh_out->num_vertices * 5 *
//     sizeof(float));
//
//     // clang-format off
//     float vertices[] = {
//         // positions         // texture coords
//          50.0f, -0.5f,  50.0f,   1.0f, 1.0f, // top right
//          50.0f, -0.5f, -50.0f,   1.0f, 0.0f, // bottom right
//         -50.0f, -0.5f, -50.0f,   0.0f, 0.0f, // bottom left
//         -50.0f, -0.5f,  50.0f,   0.0f, 1.0f, // top left
//     };
//     // clang-format on
//
//     memcpy(mesh_out->vertices, vertices, sizeof(vertices));
//
//     mesh_out->num_indices = 6;
//     mesh_out->indices =
//             (uint32_t *)malloc(mesh_out->num_indices * sizeof(uint32_t));
//     // first triangle
//     mesh_out->indices[0] = 0;
//     mesh_out->indices[1] = 1;
//     mesh_out->indices[2] = 3;
//     // second triangle
//     mesh_out->indices[3] = 1;
//     mesh_out->indices[4] = 2;
//     mesh_out->indices[5] = 3;
// }

int main() {
    int retval = 0;

    RenderConfig render_config = {
            .window_width = 1080,
            .window_height = 720,
    };

    World world;
    ecs_init(&world);

    Game game = {
            .bounds = {},
            .world = world,
    };

    vector_init(game.plugins);
    vector_init(game.resources);

    vector_append(game.plugins,
            (Plugin){.object_path = "build/libbuilder.dylib"});

    engine_run(render_config, &game);

    return retval;
}
