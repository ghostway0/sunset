#include <string.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)

#include "vfs_unix.c"

#else

#error "unsupported system"

#endif

char const *get_filename_extension(char const *filename) {
    char const *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}
