#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stddef.h>
#include <string.h>

#include <log.h>
#include <unistd.h>

#include "sunset/backend.h"
#include "sunset/camera.h"
#include "sunset/commands.h"
// #include "sunset/config.h"
#include "sunset/ecs.h"
#include "sunset/fonts.h"
#include "sunset/geometry.h"
#include "sunset/quadtree.h"
#include "sunset/scene.h"
#include "sunset/tga.h"
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

    FILE *file = fopen("skybox.tga", "rb");
    if (file == NULL) {
        log_error("Failed to open file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);

    uint8_t *data = malloc(size);

    if (data == NULL) {
        log_error("Failed to allocate memory");
        return 1;
    }

    fseek(file, 0, SEEK_SET);


    struct image skybox;
    if (load_tga_image(data, &skybox)) {
        log_error("Failed to load image");
        return 1;
    }

    fclose(file);


    /*
     *
void scene_init(struct camera camera,
        struct image skybox,
        struct effect *effects,
        size_t num_effects,
        struct box bounds,
        struct chunk *root_chunk,
        struct scene *scene_out) {
     * */

    struct chunk root_chunk = {
        .bounds = {0},
        .objects = NULL,
        .num_objects = 0,
        .lights = NULL,
        .num_lights = 0,
        .id = 0,
    };

    scene_init(camera, skybox, NULL, 0, root_chunk.bounds, &root_chunk, &scene);

    struct chunk *thing = oct_tree_query(&scene.oct_tree, (vec3){0.0f, 0.0f, 0.0f});

    log_debug("thing: " vec3_format " %zu", vec3_args(thing->bounds.min), thing->id);



    // cleanup:
    return retval;
}
