#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "sunset/errors.h"
#include "sunset/utils.h"
#include "sunset/vfs.h"

static int vfs_get_open_mode_oflag(enum vfs_open_mode mode) {
    switch (mode) {
        case VFS_OPEN_MODE_WRITE:
            return O_WRONLY;
        case VFS_OPEN_MODE_READ:
            return O_RDONLY;
        case VFS_OPEN_MODE_READ_WRITE:
            return O_RDWR;
        default:
            unreachable();
    }
}

static int vfs_map_prot_to_sys(enum vfs_map_prot prot) {
    int sys_prot = 0;
    if (prot & VFS_MAP_PROT_READ) {
        sys_prot |= PROT_READ;
    }
    if (prot & VFS_MAP_PROT_WRITE) {
        sys_prot |= PROT_WRITE;
    }
    if (prot & VFS_MAP_PROT_EXEC) {
        sys_prot |= PROT_EXEC;
    }
    return sys_prot;
}

static int vfs_map_flags_to_sys(enum vfs_map_flags flags) {
    int sys_flags = 0;
    if (flags & VFS_MAP_SHARED) {
        sys_flags |= MAP_SHARED;
    }
    if (flags & VFS_MAP_PRIVATE) {
        sys_flags |= MAP_PRIVATE;
    }
    if (flags & VFS_MAP_ANONYMOUS) {
        sys_flags |= MAP_ANONYMOUS;
    }
    return sys_flags;
}

static int vfs_seek_mode_to_sys(enum vfs_seek_mode seek_mode) {
    switch (seek_mode) {
        case VFS_SEEK_SET:
            return SEEK_SET;
        case VFS_SEEK_END:
            return SEEK_END;
        case VFS_SEEK_CUR:
            return SEEK_CUR;
        default:
            unreachable();
    }
}

size_t vfs_file_size(struct vfs_file const *file) {
    size_t current_offset = lseek(file->fd, 0, SEEK_CUR);
    size_t file_size = lseek(file->fd, 0, SEEK_END);
    lseek(file->fd, current_offset, SEEK_SET);

    return file_size;
}

int vfs_open(
        char const *path, enum vfs_open_mode mode, struct vfs_file *file_out) {
    file_out->fd = open(path, vfs_get_open_mode_oflag(mode));
    if (file_out->fd == -1) {
        return -ERROR_IO;
    }

    return 0;
}

int vfs_close(struct vfs_file *file) {
    if (close(file->fd) == -1) {
        return -ERROR_IO;
    }
    return 0;
}

int vfs_file_read(struct vfs_file *file, void *buf, size_t count) {
    ssize_t bytes_read = read(file->fd, buf, count);

    if (bytes_read == -1) {
        return -ERROR_IO;
    }

    return (int)bytes_read;
}

int vfs_file_write(struct vfs_file *file, void const *buf, size_t count) {
    ssize_t bytes_written = write(file->fd, buf, count);

    if (bytes_written == -1) {
        return -ERROR_IO;
    }

    return (int)bytes_written;
}

int vfs_map_file(struct vfs_file *file,
        enum vfs_map_prot prot,
        enum vfs_map_flags flags,
        void **addr_out) {
    size_t file_size = vfs_file_size(file);

    int sys_prot = vfs_map_prot_to_sys(prot);
    int sys_flags = vfs_map_flags_to_sys(flags);

    void *addr = mmap(NULL, file_size, sys_prot, sys_flags, file->fd, 0);
    if (addr == MAP_FAILED) {
        return -ERROR_IO;
    }

    *addr_out = addr;
    return 0;
}

size_t vfs_file_seek(
        struct vfs_file *file, enum vfs_seek_mode seek_mode, size_t offset) {
    return lseek(file->fd, offset, vfs_seek_mode_to_sys(seek_mode));
}

size_t vfs_file_get_offset(struct vfs_file *file) {
    return vfs_file_seek(file, VFS_SEEK_CUR, 0);
}

bool vfs_is_eof(struct vfs_file *file) {
    size_t current_offset = vfs_file_get_offset(file);
    if (current_offset == SIZE_FAIL) {
        return false;
    }

    return current_offset == vfs_file_size(file);
}
