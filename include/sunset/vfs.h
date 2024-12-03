#pragma once

#include <stddef.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)

struct vfs_file {
    int fd;
};

#define vfs_file_print(file, fmt) dprintf(file->fd, fmt)
#define vfs_file_printf(file, fmt, ...) dprintf(file->fd, fmt, __VA_ARGS__)

#else

#error "unsupported system"

#endif

enum vfs_open_mode {
    VFS_OPEN_MODE_WRITE = 1 << 0,
    VFS_OPEN_MODE_READ = 1 << 1,
    VFS_OPEN_MODE_READ_WRITE = VFS_OPEN_MODE_READ | VFS_OPEN_MODE_WRITE,
};

enum vfs_map_prot {
    VFS_MAP_PROT_NONE = 0,
    VFS_MAP_PROT_READ = 1 << 0,
    VFS_MAP_PROT_WRITE = 1 << 1,
    VFS_MAP_PROT_EXEC = 1 << 2,
};

enum vfs_seek_mode {
    VFS_SEEK_SET,
    VFS_SEEK_END,
    VFS_SEEK_CUR,
};

enum vfs_map_flags {
    VFS_MAP_SHARED = 1 << 0,
    VFS_MAP_PRIVATE = 1 << 1,
    VFS_MAP_ANONYMOUS = 1 << 2,
};

size_t vfs_file_size(struct vfs_file const *file);

int vfs_open(
        char const *path, enum vfs_open_mode mode, struct vfs_file *file_out);

int vfs_close(struct vfs_file *file);

int vfs_file_read(struct vfs_file *file, void *buf, size_t count);

int vfs_file_write(struct vfs_file *file, void const *buf, size_t count);

int vfs_map_file(struct vfs_file *file,
        enum vfs_map_prot prot,
        enum vfs_map_flags flags,
        void **addr_out,
        size_t *size_out);

void vfs_munmap(void *ptr, size_t size);

bool vfs_is_eof(struct vfs_file *file);

size_t vfs_file_seek(
        struct vfs_file *file, enum vfs_seek_mode seek_mode, size_t offset);

size_t vfs_file_get_offset(struct vfs_file *file);

int vfs_create_tempfile(struct vfs_file *file_out);
