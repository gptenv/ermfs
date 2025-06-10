#ifndef ERMFS_H
#define ERMFS_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct erm_file;

typedef struct erm_file erm_file;

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

#ifdef __cplusplus
}
#endif

#endif /* ERMFS_H */
