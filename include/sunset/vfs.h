#pragma once

#include <stddef.h>

#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

struct vfs_file {
    int fd;
};

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
        void **addr_out);

char const *get_filename_extesnion(char const *filename);
