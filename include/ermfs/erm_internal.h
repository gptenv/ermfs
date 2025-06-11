#ifndef ERM_INTERNAL_H
#define ERM_INTERNAL_H

#include <pthread.h>
#include <sys/types.h>
#include <stddef.h>
#ifdef ERMFS_LOCKLESS
#include <stdatomic.h>
#endif

#include "ermfs.h"
#include "ermfs_lockless.h"

/* Internal ERMFS file structure */
struct erm_file {
    void *data;
    size_t size;
    size_t capacity;
    int compressed;
    size_t original_size;
    int mode;
    char *path;
#ifdef ERMFS_LOCKLESS
    atomic_int ref_count;
#else
    int ref_count;
#endif
    pthread_mutex_t mutex;
};

typedef struct erm_file erm_file;

/* Lookup file by path in registry. Increments ref_count on success. */
erm_file *ermfs_find_file_by_path(const char *path);

/* Lock/unlock helpers for internal use */
void ermfs_lock_file(erm_file *file);
void ermfs_unlock_file(erm_file *file);

#endif /* ERM_INTERNAL_H */
