#include <string.h>
#include <sys/mman.h>

#include "sunset/byte_stream.h"
#include "sunset/color.h"
#include "sunset/errors.h"
#include "sunset/filesystem.h"
#include "sunset/images.h"
#include "sunset/io.h"
#include "sunset/tga.h"
#include "sunset/vfs.h"

int load_image_file(char const *path, struct image *image_out) {
    struct vfs_file file;
    int retval = 0;

    if ((retval = vfs_open(path, VFS_OPEN_MODE_READ, &file))) {
        return retval;
    }

    void *data;
    size_t file_size;
    if ((retval = vfs_map_file(&file,
                 VFS_MAP_PROT_READ,
                 VFS_MAP_PRIVATE,
                 &data,
                 &file_size))) {
        vfs_close(&file);
        return retval;
    }

    struct byte_stream stream;
    byte_stream_from_data(data, file_size, &stream);

    struct reader reader = {.read = (read_fn)byte_stream_read, .ctx = &stream};

    if (strcmp(get_filename_extesnion(path), ".tga")) {
        retval = tga_load_image(&reader, image_out);
    } else {
        retval = -ERROR_INVALID_ARGUMENTS;
    }

    vfs_close(&file);
    vfs_munmap(data, file_size);
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
        printf("\033[%u;%uH", (uint32_t)pos.y + (uint32_t)y, (uint32_t)pos.x);

        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel = color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }

    printf("\033[%lu;%luH", (uint32_t)pos.y + image->h, (size_t)1);
}

void image_destroy(struct image *image) {
    free(image->pixels);
}
