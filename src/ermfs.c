#include "ermfs/ermfs.h"
#include "ermfs/erm_alloc.h"

#include <stdlib.h>
#include <string.h>

struct erm_file {
    void *data;
    size_t size;
    size_t capacity;
};

erm_file *ermfs_create(size_t initial_size) {
    erm_file *file = malloc(sizeof(*file));
    if (!file) {
        return NULL;
    }
    file->data = erm_alloc(initial_size);
    if (!file->data) {
        free(file);
        return NULL;
    }
    file->size = 0;
    file->capacity = initial_size;
    return file;
}

ssize_t ermfs_write(erm_file *file, const void *data, size_t len) {
    if (!file || !data) {
        return -1;
    }
    size_t required = file->size + len;
    if (required > file->capacity) {
        size_t newcap = file->capacity * 2;
        if (newcap < required) {
            newcap = required;
        }
        void *newdata = erm_resize(file->data, file->capacity, newcap);
        if (!newdata) {
            return -1;
        }
        file->data = newdata;
        file->capacity = newcap;
    }
    memcpy((char *)file->data + file->size, data, len);
    file->size += len;
    return (ssize_t)len;
}

void *ermfs_data(erm_file *file) {
    return file ? file->data : NULL;
}

size_t ermfs_size(erm_file *file) {
    return file ? file->size : 0;
}

void ermfs_close(erm_file *file) {
    if (!file) {
        return;
    }
    erm_free(file->data, file->capacity);
    free(file);
}
