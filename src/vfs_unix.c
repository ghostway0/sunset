#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "sunset/errors.h"
#include "sunset/io.h"
#include "internal/utils.h"
#include "sunset/vfs.h"

#ifdef __linux__

#include <linux/limits.h>

#endif

static int vfs_get_open_mode_oflag(VfsOpenMode mode) {
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

static int vfs_map_prot_to_sys(VfsMapProt prot) {
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

static int vfs_map_flags_to_sys(VfsMapFlags flags) {
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

static int vfs_seek_mode_to_sys(VfsSeekMode seek_mode) {
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

size_t vfs_file_size(VfsFile const *file) {
    size_t current_offset = lseek(file->fd, 0, SEEK_CUR);
    size_t file_size = lseek(file->fd, 0, SEEK_END);
    lseek(file->fd, current_offset, SEEK_SET);

    return file_size;
}

int vfs_open(char const *path, VfsOpenMode mode, VfsFile *file_out) {
    file_out->fd = open(path, vfs_get_open_mode_oflag(mode));
    if (file_out->fd == -1) {
        return -ERROR_IO;
    }

    return 0;
}

int vfs_close(VfsFile *file) {
    if (close(file->fd) == -1) {
        return -ERROR_IO;
    }

    file->fd = -1;

    return 0;
}

ssize_t vfs_file_read(VfsFile *file, size_t count, void *buf) {
    ssize_t bytes_read = read(file->fd, buf, count);

    if (bytes_read == -1) {
        return -ERROR_IO;
    }

    return bytes_read;
}

ssize_t vfs_file_write(VfsFile *file, void const *buf, size_t count) {
    ssize_t bytes_written = write(file->fd, buf, count);

    if (bytes_written == -1) {
        return -ERROR_IO;
    }

    return bytes_written;
}

int vfs_map_file(VfsFile *file,
        VfsMapProt prot,
        VfsMapFlags flags,
        void **addr_out,
        size_t *size_out) {
    assert(addr_out != NULL);

    size_t file_size = vfs_file_size(file);

    int sys_prot = vfs_map_prot_to_sys(prot);
    int sys_flags = vfs_map_flags_to_sys(flags);

    void *addr = mmap(NULL, file_size, sys_prot, sys_flags, file->fd, 0);
    if (addr == MAP_FAILED) {
        return -ERROR_IO;
    }

    if (size_out) {
        *size_out = file_size;
    }

    *addr_out = addr;
    return 0;
}

size_t vfs_file_seek(VfsFile *file, VfsSeekMode seek_mode, size_t offset) {
    return lseek(file->fd, offset, vfs_seek_mode_to_sys(seek_mode));
}

size_t vfs_file_get_offset(VfsFile *file) {
    return vfs_file_seek(file, VFS_SEEK_CUR, 0);
}

bool vfs_is_eof(VfsFile *file) {
    size_t current_offset = vfs_file_get_offset(file);
    if (current_offset == SIZE_FAIL) {
        return false;
    }

    return current_offset == vfs_file_size(file);
}

int vfs_create_tempfile(VfsFile *file_out) {
    char filename[] = "/tmp/sunset-temp.XXXXXX";
    int fd = mkstemp(filename);
    if (fd == -1) {
        return -ERROR_IO;
    }

    file_out->fd = fd;

    return 0;
}

void vfs_munmap(void *ptr, size_t size) {
    munmap(ptr, size);
}

void vfs_get_stdout(VfsFile *file_out) {
    file_out->fd = STDOUT_FILENO;
}

void vfs_get_stderr(VfsFile *file_out) {
    file_out->fd = STDERR_FILENO;
}

void vfs_get_stdin(VfsFile *file_out) {
    file_out->fd = STDIN_FILENO;
}

Writer vfs_file_writer(VfsFile *file) {
    return (Writer){.ctx = file, .write = (WriteFn)vfs_file_write};
}

Reader vfs_file_reader(VfsFile *file) {
    return (Reader){.ctx = file, .read = (ReadFn)vfs_file_read};
}
