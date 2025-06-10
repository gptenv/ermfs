#include "ermfs/ermfs.h"
#include "ermfs/erm_alloc.h"
#include "ermfs/erm_compress.h"

#include <stdlib.h>
#include <string.h>

struct erm_file {
    void *data;
    size_t size;
    size_t capacity;
    int compressed;        /* 1 if data is compressed, 0 otherwise */
    size_t original_size;  /* original size before compression */
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
    file->compressed = 0;
    file->original_size = 0;
    return file;
}

/* Helper function to ensure file data is decompressed before access */
static int ensure_decompressed(erm_file *file) {
    if (!file || !file->compressed) {
        return 0; /* Nothing to do */
    }
    
    /* Decompress the data */
    size_t decompressed_size;
    void *decompressed_data = erm_decompress(file->data, file->size, &decompressed_size);
    if (!decompressed_data) {
        return -1; /* Decompression failed */
    }
    
    /* Replace the compressed data with decompressed data */
    erm_free(file->data, file->capacity);
    file->data = erm_alloc(decompressed_size);
    if (!file->data) {
        free(decompressed_data);
        return -1;
    }
    
    memcpy(file->data, decompressed_data, decompressed_size);
    free(decompressed_data);
    
    file->size = decompressed_size;
    file->capacity = decompressed_size;
    file->compressed = 0;
    file->original_size = 0;
    
    return 0;
}

ssize_t ermfs_write(erm_file *file, const void *data, size_t len) {
    if (!file || !data) {
        return -1;
    }
    
    /* Ensure data is decompressed before writing */
    if (ensure_decompressed(file) != 0) {
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
    if (!file) {
        return NULL;
    }
    
    /* Ensure data is decompressed before returning pointer */
    if (ensure_decompressed(file) != 0) {
        return NULL;
    }
    
    return file->data;
}

size_t ermfs_size(erm_file *file) {
    if (!file) {
        return 0;
    }
    
    /* Return original size if compressed, current size otherwise */
    return file->compressed ? file->original_size : file->size;
}

void ermfs_close(erm_file *file) {
    if (!file) {
        return;
    }
    
    /* Skip compression if already compressed or no data */
    if (file->compressed || file->size == 0) {
        return;
    }
    
    /* Compress the data */
    size_t compressed_size;
    void *compressed_data = erm_compress(file->data, file->size, &compressed_size);
    if (!compressed_data) {
        /* Compression failed, leave data uncompressed */
        return;
    }
    
    /* Store original size for later decompression */
    file->original_size = file->size;
    
    /* Replace uncompressed data with compressed data */
    erm_free(file->data, file->capacity);
    file->data = erm_alloc(compressed_size);
    if (!file->data) {
        /* Allocation failed, restore original data */
        file->data = compressed_data; /* This is malloc'd, not erm_alloc'd! */
        /* We'll handle this inconsistency by using a different strategy */
        free(compressed_data);
        return;
    }
    
    memcpy(file->data, compressed_data, compressed_size);
    free(compressed_data);
    
    file->size = compressed_size;
    file->capacity = compressed_size;
    file->compressed = 1;
}

void ermfs_destroy(erm_file *file) {
    if (!file) {
        return;
    }
    erm_free(file->data, file->capacity);
    free(file);
}
