#define _GNU_SOURCE
#include "ermfs/ermfd.h"
#include "ermfs/erm_internal.h"
#include "ermfs/ermfs.h"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int ermfs_export_memfd(const char *path, int flags) {
    (void)flags;
    if (!path) {
        errno = EINVAL;
        return -1;
    }

    /* Lookup file object */
    erm_file *file = ermfs_find_file_by_path(path);
    if (!file) {
        errno = ENOENT;
        return -1;
    }

    int fd = memfd_create(path, MFD_CLOEXEC);
    if (fd == -1) {
        ermfs_destroy(file);
        return -1;
    }

    /* Snapshot file contents while holding lock */
    ermfs_lock_file(file);
    void *data = ermfs_data(file);
    size_t size = ermfs_size(file);
    if (!data) {
        ermfs_unlock_file(file);
        close(fd);
        ermfs_destroy(file);
        errno = EIO;
        return -1;
    }

    ssize_t off = 0;
    while (off < (ssize_t)size) {
        ssize_t written = write(fd, (char *)data + off, size - off);
        if (written <= 0) {
            ermfs_unlock_file(file);
            close(fd);
            ermfs_destroy(file);
            return -1;
        }
        off += written;
    }
    lseek(fd, 0, SEEK_SET);
    ermfs_unlock_file(file);
    ermfs_destroy(file);

    return fd;
}
