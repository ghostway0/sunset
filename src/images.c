#include <string.h>
#include <sys/mman.h>

#include "sunset/color.h"
#include "sunset/errors.h"
#include "sunset/images.h"
#include "sunset/tga.h"
#include "sunset/filesystem.h"

int load_image_file(char const *path, struct image *image_out) {
    struct vfs_file file;
    int retval = 0;

    if ((retval = vfs_open(path, VFS_OPEN_MODE_READ, &file))) {
        return retval;
    }

    void *data;
    if ((retval = vfs_map_file(
                 &file, VFS_MAP_PROT_READ, VFS_MAP_PRIVATE, &data))) {
        return retval;
    }

    if (strcmp(get_filename_extesnion(path), ".tga")) {
        retval = load_tga_image(data, image_out);
    } else {
        retval = -ERROR_INVALID_ARGUMENTS;
    }

    return retval;
}

void show_image_grayscale(struct image const *image) {
    for (size_t y = 0; y < image->h; y++) {
        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel = color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }
}

void show_image_grayscale_at(struct image const *image, struct point pos) {
    for (size_t y = 0; y < image->h; y++) {
        printf("\033[%u;%uH", pos.y + (uint32_t)y, pos.x);

        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel = color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }

    printf("\033[%lu;%luH", pos.y + image->h, (size_t)1);
}

void image_deinit(struct image *image) {
    free(image->pixels);
}
