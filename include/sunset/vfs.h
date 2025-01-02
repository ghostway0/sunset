#pragma once

#include "sunset/utils.h"
#include <stddef.h>

typedef struct Writer Writer;
typedef struct Reader Reader;

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)

typedef struct VfsFile {
    int fd;
} VfsFile;

#define vfs_file_print(file, fmt) dprintf(file->fd, fmt)
#define vfs_file_printf(file, fmt, ...) dprintf(file->fd, fmt, __VA_ARGS__)

#else

#error "unsupported system"

#endif

typedef enum VfsOpenMode {
    VFS_OPEN_MODE_WRITE = 1 << 0,
    VFS_OPEN_MODE_READ = 1 << 1,
    VFS_OPEN_MODE_READ_WRITE = VFS_OPEN_MODE_READ | VFS_OPEN_MODE_WRITE,
} VfsOpenMode;

typedef enum VfsMapProt {
    VFS_MAP_PROT_NONE = 0,
    VFS_MAP_PROT_READ = 1 << 0,
    VFS_MAP_PROT_WRITE = 1 << 1,
    VFS_MAP_PROT_EXEC = 1 << 2,
} VfsMapProt;

typedef enum VfsSeekMode {
    VFS_SEEK_SET,
    VFS_SEEK_END,
    VFS_SEEK_CUR,
} VfsSeekMode;

typedef enum VfsMapFlags {
    VFS_MAP_SHARED = 1 << 0,
    VFS_MAP_PRIVATE = 1 << 1,
    VFS_MAP_ANONYMOUS = 1 << 2,
} VfsMapFlags;

size_t vfs_file_size(VfsFile const *file);

int vfs_open(char const *path, VfsOpenMode mode, VfsFile *file_out);

int vfs_close(VfsFile *file);

ssize_t vfs_file_read(VfsFile *file, size_t count, void *buf);

ssize_t vfs_file_write(VfsFile *file, void const *buf, size_t count);

int vfs_map_file(VfsFile *file,
        VfsMapProt prot,
        VfsMapFlags flags,
        void **addr_out,
        size_t *size_out);

void vfs_munmap(void *ptr, size_t size);

bool vfs_is_eof(VfsFile *file);

size_t vfs_file_seek(VfsFile *file, VfsSeekMode seek_mode, size_t offset);

size_t vfs_file_get_offset(VfsFile *file);

int vfs_create_tempfile(VfsFile *file_out);

void vfs_get_stdout(VfsFile *file_out);

void vfs_get_stderr(VfsFile *file_out);

void vfs_get_stdin(VfsFile *file_out);

Writer vfs_file_writer(VfsFile *file);

Reader vfs_file_reader(VfsFile *file);
