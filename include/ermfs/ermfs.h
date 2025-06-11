#ifndef ERMFS_H
#define ERMFS_H

#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* File descriptor type for VFS operations */
typedef int ermfs_fd_t;

/* File statistics structure */
struct ermfs_stat {
    size_t size;        /* Current file size */
    int compressed;     /* 1 if file is compressed, 0 otherwise */
    int mode;           /* File access mode */
};

struct erm_file;

typedef struct erm_file erm_file;

/* === VFS API Functions === */

/* Open a file with path and flags, returns file descriptor or -1 on error */
ermfs_fd_t ermfs_open(const char *path, int flags);

/* Read data from file descriptor, returns bytes read or -1 on error */
ssize_t ermfs_read(ermfs_fd_t fd, void *buf, size_t len);

/* Write data to file descriptor, returns bytes written or -1 on error */
ssize_t ermfs_write_fd(ermfs_fd_t fd, const void *buf, size_t len);

/* Seek to position in file, returns new position or -1 on error */
off_t ermfs_seek(ermfs_fd_t fd, off_t offset, int whence);

/* Get file statistics, returns 0 on success or -1 on error */
int ermfs_stat(ermfs_fd_t fd, struct ermfs_stat *stat);

/* Close file descriptor, returns 0 on success or -1 on error */
int ermfs_close_fd(ermfs_fd_t fd);

/* Truncate file to specified size, returns 0 on success or -1 on error */
int ermfs_truncate(ermfs_fd_t fd, off_t length);

/* === Legacy Direct File API === */

/* Create a new in-memory file with specified initial capacity. */
erm_file *ermfs_create(size_t initial_size);

/* Write data to the file, growing as needed. Returns bytes written or -1. */
ssize_t ermfs_write(erm_file *file, const void *data, size_t len);

/* Obtain a pointer to the internal buffer. */
void *ermfs_data(erm_file *file);

/* Current size of the file. */
size_t ermfs_size(erm_file *file);

/* Close and free the file. */
void ermfs_close(erm_file *file);

/* Destroy the file and free all memory. */
void ermfs_destroy(erm_file *file);

#ifdef __cplusplus
}
#endif

#endif /* ERMFS_H */
