#include <string.h>

#include "sunset/byte_stream.h"
#include "sunset/errors.h"
#include "sunset/filesystem.h"
#include "sunset/geometry.h"
#include "sunset/io.h"
#include "sunset/tga.h"
#include "sunset/vfs.h"

#include "sunset/images.h"

Color color_from_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (Color){r, g, b, 255};
}

Color color_from_bytes(uint8_t *bytes, size_t count) {
    switch (count) {
        case 1:
            return color_from_grayscale(bytes[0]);
        case 2:
            return color_from_16bit(*(uint16_t *)bytes);
        case 3:
            return color_from_rgb(bytes[0], bytes[1], bytes[2]);
        case 4:
            return (Color){bytes[0], bytes[1], bytes[2], bytes[3]};
        default:
            unreachable();
    }
}

Color color_from_16bit(uint16_t color) {
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);

    return (Color){r, g, b, 255};
}

Color color_from_hex(char const *hex_str) {
    uint32_t hex;

    sscanf(hex_str, "#%x", &hex);
    return (Color){
            .r = (hex >> 16) & 0xFF,
            .g = (hex >> 8) & 0xFF,
            .b = hex & 0xFF,
            .a = 255,
    };
}

bool colors_equal(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

uint8_t color_to_grayscale(Color color) {
    return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

Color color_from_grayscale(uint8_t value) {
    return (Color){value, value, value, 255};
}

int load_image_file(char const *path, Image *image_out) {
    VfsFile file;
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

    ByteStream stream;
    bstream_from_ro(data, file_size, &stream);

    Reader reader = {.read = (ReadFn)bstream_read, .ctx = &stream};

    if (strcmp(get_filename_extension(path), ".tga")) {
        retval = tga_load_image(&reader, image_out);
    } else {
        retval = -ERROR_INVALID_ARGUMENTS;
    }

    vfs_close(&file);
    vfs_munmap(data, file_size);
    return retval;
}

void show_image_grayscale(Image const *image) {
    for (size_t y = 0; y < image->h; y++) {
        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel =
                    color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }
}

void show_image_grayscale_at(Image const *image, Point pos) {
    for (size_t y = 0; y < image->h; y++) {
        printf("\033[%u;%uH",
                (uint32_t)pos.y + (uint32_t)y,
                (uint32_t)pos.x);

        for (size_t x = 0; x < image->w; x++) {
            uint8_t pixel =
                    color_to_grayscale(image->pixels[y * image->w + x]);
            printf("%c", " .:-=+*#"[pixel / 32]);
        }
        printf("\n");
    }

    printf("\033[%lu;%luH", (uint32_t)pos.y + image->h, (size_t)1);
}

void image_destroy(Image *image) {
    free(image->pixels);
}

// void image_convert(Image const *image, Image *image_out) {}

int image_slice(Image const *image, Rect bounds, Image *sliced_out) {
    // TODO: add bounds checking

    sliced_out->w = bounds.w;
    sliced_out->h = bounds.h;

    sliced_out->pixels =
            image->pixels + (size_t)bounds.y * image->w + (size_t)bounds.x;

    return 0;
}
