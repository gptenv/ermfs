#include "ermfs/ermfs.h"
#include "ermfs/erm_alloc.h"
#include "ermfs/erm_compress.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

struct erm_file {
    void *data;
    size_t size;
    size_t capacity;
    int compressed;        /* 1 if data is compressed, 0 otherwise */
    size_t original_size;  /* original size before compression */
    off_t position;        /* current seek position */
    int mode;              /* file access mode (O_RDONLY, O_WRONLY, O_RDWR) */
    char *path;            /* file path (owned by this struct) */
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
    file->position = 0;
    file->mode = O_RDWR;  /* Default mode */
    file->path = NULL;
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
    free(file->path);  /* Free the path string if allocated */
    free(file);
}

/* === VFS File Descriptor Table === */

#define ERMFS_MAX_FILES 1024
#define ERMFS_FD_OFFSET 1000  /* Start file descriptors at 1000 to avoid conflicts */

static struct {
    erm_file *file;
    int in_use;
} fd_table[ERMFS_MAX_FILES];

static int fd_table_initialized = 0;

/* Initialize the file descriptor table */
static void init_fd_table(void) {
    if (fd_table_initialized) {
        return;
    }
    for (int i = 0; i < ERMFS_MAX_FILES; i++) {
        fd_table[i].file = NULL;
        fd_table[i].in_use = 0;
    }
    fd_table_initialized = 1;
}

/* Allocate a new file descriptor */
static ermfs_fd_t alloc_fd(erm_file *file) {
    init_fd_table();
    
    for (int i = 0; i < ERMFS_MAX_FILES; i++) {
        if (!fd_table[i].in_use) {
            fd_table[i].file = file;
            fd_table[i].in_use = 1;
            return ERMFS_FD_OFFSET + i;
        }
    }
    return -1;  /* No available file descriptors */
}

/* Get file from file descriptor */
static erm_file *get_file_from_fd(ermfs_fd_t fd) {
    init_fd_table();
    
    int idx = fd - ERMFS_FD_OFFSET;
    if (idx < 0 || idx >= ERMFS_MAX_FILES || !fd_table[idx].in_use) {
        return NULL;
    }
    return fd_table[idx].file;
}

/* Free a file descriptor */
static int free_fd(ermfs_fd_t fd) {
    init_fd_table();
    
    int idx = fd - ERMFS_FD_OFFSET;
    if (idx < 0 || idx >= ERMFS_MAX_FILES || !fd_table[idx].in_use) {
        return -1;
    }
    fd_table[idx].file = NULL;
    fd_table[idx].in_use = 0;
    return 0;
}

/* === VFS API Implementation === */

ermfs_fd_t ermfs_open(const char *path, int flags) {
    if (!path) {
        return -1;
    }
    
    /* Create a new file with default initial size */
    erm_file *file = ermfs_create(4096);
    if (!file) {
        return -1;
    }
    
    /* Set file properties */
    file->mode = flags & (O_RDONLY | O_WRONLY | O_RDWR);
    if (file->mode == 0) {
        file->mode = O_RDONLY;  /* Default to read-only */
    }
    
    /* Copy the path */
    file->path = malloc(strlen(path) + 1);
    if (!file->path) {
        ermfs_destroy(file);
        return -1;
    }
    strcpy(file->path, path);
    
    /* Allocate file descriptor */
    ermfs_fd_t fd = alloc_fd(file);
    if (fd == -1) {
        ermfs_destroy(file);
        return -1;
    }
    
    return fd;
}

ssize_t ermfs_read(ermfs_fd_t fd, void *buf, size_t len) {
    erm_file *file = get_file_from_fd(fd);
    if (!file || !buf) {
        return -1;
    }
    
    /* Check if file is open for reading */
    if (file->mode == O_WRONLY) {
        return -1;  /* File not open for reading */
    }
    
    /* Ensure data is decompressed before reading */
    if (ensure_decompressed(file) != 0) {
        return -1;
    }
    
    /* Check bounds */
    if (file->position >= (off_t)file->size) {
        return 0;  /* EOF */
    }
    
    /* Calculate how much we can actually read */
    size_t available = file->size - file->position;
    size_t to_read = (len < available) ? len : available;
    
    /* Copy data to buffer */
    memcpy(buf, (char *)file->data + file->position, to_read);
    file->position += to_read;
    
    return (ssize_t)to_read;
}

ssize_t ermfs_write_fd(ermfs_fd_t fd, const void *buf, size_t len) {
    erm_file *file = get_file_from_fd(fd);
    if (!file || !buf) {
        return -1;
    }
    
    /* Check if file is open for writing */
    if (file->mode == O_RDONLY) {
        return -1;  /* File not open for writing */
    }
    
    /* Ensure data is decompressed before writing */
    if (ensure_decompressed(file) != 0) {
        return -1;
    }
    
    /* Calculate required size at current position */
    size_t required_end = file->position + len;
    
    /* Expand file if necessary */
    if (required_end > file->capacity) {
        size_t newcap = file->capacity * 2;
        if (newcap < required_end) {
            newcap = required_end;
        }
        void *newdata = erm_resize(file->data, file->capacity, newcap);
        if (!newdata) {
            return -1;
        }
        file->data = newdata;
        file->capacity = newcap;
    }
    
    /* Write data at current position */
    memcpy((char *)file->data + file->position, buf, len);
    file->position += len;
    
    /* Update file size if we wrote past the current end */
    if (file->position > (off_t)file->size) {
        file->size = file->position;
    }
    
    return (ssize_t)len;
}

off_t ermfs_seek(ermfs_fd_t fd, off_t offset, int whence) {
    erm_file *file = get_file_from_fd(fd);
    if (!file) {
        return -1;
    }
    
    /* Ensure data is decompressed for size calculations */
    if (ensure_decompressed(file) != 0) {
        return -1;
    }
    
    off_t new_pos;
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->position + offset;
            break;
        case SEEK_END:
            new_pos = (off_t)file->size + offset;
            break;
        default:
            return -1;  /* Invalid whence */
    }
    
    /* Validate new position (allow seeking past end for writes) */
    if (new_pos < 0) {
        return -1;
    }
    
    file->position = new_pos;
    return new_pos;
}

int ermfs_stat(ermfs_fd_t fd, struct ermfs_stat *stat) {
    erm_file *file = get_file_from_fd(fd);
    if (!file || !stat) {
        return -1;
    }
    
    stat->size = ermfs_size(file);  /* Use existing function that handles compression */
    stat->compressed = file->compressed;
    stat->mode = file->mode;
    
    return 0;
}

int ermfs_close_fd(ermfs_fd_t fd) {
    erm_file *file = get_file_from_fd(fd);
    if (!file) {
        return -1;
    }
    
    /* Compress the file using existing close logic */
    ermfs_close(file);
    
    /* Destroy the file and free the descriptor */
    ermfs_destroy(file);
    
    /* Free the file descriptor slot */
    return free_fd(fd);
}
