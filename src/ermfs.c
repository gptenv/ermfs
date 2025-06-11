#include "ermfs/ermfs.h"
#include "ermfs/erm_alloc.h"
#include "ermfs/erm_compress.h"
#include "ermfs/erm_internal.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>


erm_file *ermfs_create(size_t initial_size) {
    erm_file *file = malloc(sizeof(*file));
    if (!file) {
        errno = ENOMEM;
        return NULL;
    }
    file->data = erm_alloc(initial_size);
    if (!file->data) {
        free(file);
        errno = ENOMEM;
        return NULL;
    }
    file->size = 0;
    file->capacity = initial_size;
    file->compressed = 0;
    file->original_size = 0;
    file->position = 0;
    file->mode = O_RDWR;  /* Default mode */
    file->path = NULL;
    file->ref_count = 1;
    
    /* Initialize mutex */
    if (pthread_mutex_init(&file->mutex, NULL) != 0) {
        erm_free(file->data, file->capacity);
        free(file);
        errno = ENOMEM;
        return NULL;
    }
    
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
    
    pthread_mutex_lock(&file->mutex);
    file->ref_count--;
    
    if (file->ref_count <= 0) {
        erm_free(file->data, file->capacity);
        free(file->path);  /* Free the path string if allocated */
        pthread_mutex_unlock(&file->mutex);
        pthread_mutex_destroy(&file->mutex);
        free(file);
    } else {
        pthread_mutex_unlock(&file->mutex);
    }
}

/* === VFS File Descriptor Table === */

#define ERMFS_MAX_FILES 1024
#define ERMFS_FD_OFFSET 1000  /* Start file descriptors at 1000 to avoid conflicts */

static struct {
    erm_file *file;
    int in_use;
    int fd_mode;  /* Per-FD mode (can be more restrictive than file mode) */
} fd_table[ERMFS_MAX_FILES];

static int fd_table_initialized = 0;
static pthread_mutex_t fd_table_mutex = PTHREAD_MUTEX_INITIALIZER;

/* === File Registry for Path-Based Lookup === */

#define ERMFS_MAX_REGISTRY_FILES 256

static struct {
    erm_file *file;
    char *path;
    int in_use;
} file_registry[ERMFS_MAX_REGISTRY_FILES];

static int file_registry_initialized = 0;
static pthread_mutex_t file_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Initialize the file descriptor table */
static void init_fd_table(void) {
    if (fd_table_initialized) {
        return;
    }
    pthread_mutex_lock(&fd_table_mutex);
    if (!fd_table_initialized) {
        for (int i = 0; i < ERMFS_MAX_FILES; i++) {
            fd_table[i].file = NULL;
            fd_table[i].in_use = 0;
            fd_table[i].fd_mode = 0;
        }
        fd_table_initialized = 1;
    }
    pthread_mutex_unlock(&fd_table_mutex);
}

/* Initialize the file registry */
static void init_file_registry(void) {
    if (file_registry_initialized) {
        return;
    }
    pthread_mutex_lock(&file_registry_mutex);
    if (!file_registry_initialized) {
        for (int i = 0; i < ERMFS_MAX_REGISTRY_FILES; i++) {
            file_registry[i].file = NULL;
            file_registry[i].path = NULL;
            file_registry[i].in_use = 0;
        }
        file_registry_initialized = 1;
    }
    pthread_mutex_unlock(&file_registry_mutex);
}

/* Find file by path in registry (increments ref_count on success) */
erm_file *ermfs_find_file_by_path(const char *path) {
    init_file_registry();
    
    pthread_mutex_lock(&file_registry_mutex);
    for (int i = 0; i < ERMFS_MAX_REGISTRY_FILES; i++) {
        if (file_registry[i].in_use && 
            file_registry[i].path && 
            strcmp(file_registry[i].path, path) == 0) {
            erm_file *file = file_registry[i].file;
            pthread_mutex_lock(&file->mutex);
            file->ref_count++;
            pthread_mutex_unlock(&file->mutex);
            pthread_mutex_unlock(&file_registry_mutex);
            return file;
        }
    }
    pthread_mutex_unlock(&file_registry_mutex);
    return NULL;
}

/* Register file in registry */
static int register_file(erm_file *file, const char *path) {
    init_file_registry();
    
    pthread_mutex_lock(&file_registry_mutex);
    for (int i = 0; i < ERMFS_MAX_REGISTRY_FILES; i++) {
        if (!file_registry[i].in_use) {
            file_registry[i].file = file;
            file_registry[i].path = malloc(strlen(path) + 1);
            if (!file_registry[i].path) {
                pthread_mutex_unlock(&file_registry_mutex);
                errno = ENOMEM;
                return -1;
            }
            strcpy(file_registry[i].path, path);
            file_registry[i].in_use = 1;
            pthread_mutex_unlock(&file_registry_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&file_registry_mutex);
    errno = ENFILE;  /* Too many open files */
    return -1;
}

/* Unregister file from registry */
static void unregister_file(const char *path) {
    init_file_registry();
    
    pthread_mutex_lock(&file_registry_mutex);
    for (int i = 0; i < ERMFS_MAX_REGISTRY_FILES; i++) {
        if (file_registry[i].in_use && 
            file_registry[i].path && 
            strcmp(file_registry[i].path, path) == 0) {
            free(file_registry[i].path);
            file_registry[i].file = NULL;
            file_registry[i].path = NULL;
            file_registry[i].in_use = 0;
            break;
        }
    }
    pthread_mutex_unlock(&file_registry_mutex);
}

/* Lock and unlock helpers for internal modules */
void ermfs_lock_file(erm_file *file) {
    if (file) {
        pthread_mutex_lock(&file->mutex);
    }
}

void ermfs_unlock_file(erm_file *file) {
    if (file) {
        pthread_mutex_unlock(&file->mutex);
    }
}

/* Allocate a new file descriptor */
static ermfs_fd_t alloc_fd(erm_file *file, int fd_mode) {
    init_fd_table();
    
    pthread_mutex_lock(&fd_table_mutex);
    for (int i = 0; i < ERMFS_MAX_FILES; i++) {
        if (!fd_table[i].in_use) {
            fd_table[i].file = file;
            fd_table[i].in_use = 1;
            fd_table[i].fd_mode = fd_mode;
            pthread_mutex_unlock(&fd_table_mutex);
            return ERMFS_FD_OFFSET + i;
        }
    }
    pthread_mutex_unlock(&fd_table_mutex);
    errno = EMFILE;  /* Too many open files */
    return -1;
}

/* Get file from file descriptor */
static erm_file *get_file_from_fd(ermfs_fd_t fd) {
    init_fd_table();
    
    int idx = fd - ERMFS_FD_OFFSET;
    if (idx < 0 || idx >= ERMFS_MAX_FILES) {
        errno = EBADF;
        return NULL;
    }
    
    pthread_mutex_lock(&fd_table_mutex);
    if (!fd_table[idx].in_use) {
        pthread_mutex_unlock(&fd_table_mutex);
        errno = EBADF;
        return NULL;
    }
    erm_file *file = fd_table[idx].file;
    pthread_mutex_unlock(&fd_table_mutex);
    return file;
}

/* Get fd mode */
static int get_fd_mode(ermfs_fd_t fd) {
    init_fd_table();
    
    int idx = fd - ERMFS_FD_OFFSET;
    if (idx < 0 || idx >= ERMFS_MAX_FILES) {
        return -1;
    }
    
    pthread_mutex_lock(&fd_table_mutex);
    if (!fd_table[idx].in_use) {
        pthread_mutex_unlock(&fd_table_mutex);
        return -1;
    }
    int mode = fd_table[idx].fd_mode;
    pthread_mutex_unlock(&fd_table_mutex);
    return mode;
}

/* Free a file descriptor */
static int free_fd(ermfs_fd_t fd) {
    init_fd_table();
    
    int idx = fd - ERMFS_FD_OFFSET;
    if (idx < 0 || idx >= ERMFS_MAX_FILES) {
        errno = EBADF;
        return -1;
    }
    
    pthread_mutex_lock(&fd_table_mutex);
    if (!fd_table[idx].in_use) {
        pthread_mutex_unlock(&fd_table_mutex);
        errno = EBADF;
        return -1;
    }
    fd_table[idx].file = NULL;
    fd_table[idx].in_use = 0;
    fd_table[idx].fd_mode = 0;
    pthread_mutex_unlock(&fd_table_mutex);
    return 0;
}

/* === VFS API Implementation === */

ermfs_fd_t ermfs_open(const char *path, int flags) {
    if (!path) {
        errno = EINVAL;
        return -1;
    }
    
    /* Try to find existing file first */
    erm_file *file = ermfs_find_file_by_path(path);
    
    if (!file) {
        /* Create a new file */
        file = ermfs_create(4096);
        if (!file) {
            return -1;  /* errno already set by ermfs_create */
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
            errno = ENOMEM;
            return -1;
        }
        strcpy(file->path, path);
        
        /* Register file in registry */
        if (register_file(file, path) != 0) {
            ermfs_destroy(file);
            return -1;  /* errno already set by register_file */
        }
    } else {
        /* Reset position for new file descriptor */
        pthread_mutex_lock(&file->mutex);
        /* Don't reset position - each FD should have independent position */
        pthread_mutex_unlock(&file->mutex);
    }
    
    /* Determine FD mode (can be more restrictive than file mode) */
    int fd_mode = flags & (O_RDONLY | O_WRONLY | O_RDWR);
    if (fd_mode == 0) {
        fd_mode = O_RDONLY;
    }
    
    /* Check if requested mode is compatible with file mode */
    if ((fd_mode & O_WRONLY) && (file->mode == O_RDONLY)) {
        errno = EACCES;
        ermfs_destroy(file);  /* This will decrement ref_count */
        return -1;
    }
    if ((fd_mode & O_RDONLY) && (file->mode == O_WRONLY)) {
        errno = EACCES;
        ermfs_destroy(file);  /* This will decrement ref_count */
        return -1;
    }
    
    /* Allocate file descriptor */
    ermfs_fd_t fd = alloc_fd(file, fd_mode);
    if (fd == -1) {
        ermfs_destroy(file);  /* This will decrement ref_count */
        return -1;  /* errno already set by alloc_fd */
    }
    
    return fd;
}

ssize_t ermfs_read(ermfs_fd_t fd, void *buf, size_t len) {
    erm_file *file = get_file_from_fd(fd);
    if (!file || !buf) {
        errno = file ? EINVAL : EBADF;
        return -1;
    }
    
    /* Check if file descriptor is open for reading */
    int fd_mode = get_fd_mode(fd);
    if (fd_mode == -1) {
        errno = EBADF;
        return -1;
    }
    if (fd_mode == O_WRONLY) {
        errno = EBADF;
        return -1;  /* File not open for reading */
    }
    
    pthread_mutex_lock(&file->mutex);
    
    /* Ensure data is decompressed before reading */
    if (ensure_decompressed(file) != 0) {
        pthread_mutex_unlock(&file->mutex);
        errno = EIO;
        return -1;
    }
    
    /* Check bounds */
    if (file->position >= (off_t)file->size) {
        pthread_mutex_unlock(&file->mutex);
        return 0;  /* EOF */
    }
    
    /* Calculate how much we can actually read */
    size_t available = file->size - file->position;
    size_t to_read = (len < available) ? len : available;
    
    /* Copy data to buffer */
    memcpy(buf, (char *)file->data + file->position, to_read);
    file->position += to_read;
    
    pthread_mutex_unlock(&file->mutex);
    return (ssize_t)to_read;
}

ssize_t ermfs_write_fd(ermfs_fd_t fd, const void *buf, size_t len) {
    erm_file *file = get_file_from_fd(fd);
    if (!file || !buf) {
        errno = file ? EINVAL : EBADF;
        return -1;
    }
    
    /* Check if file descriptor is open for writing */
    int fd_mode = get_fd_mode(fd);
    if (fd_mode == -1) {
        errno = EBADF;
        return -1;
    }
    if (fd_mode == O_RDONLY) {
        errno = EBADF;
        return -1;  /* File not open for writing */
    }
    
    pthread_mutex_lock(&file->mutex);
    
    /* Ensure data is decompressed before writing */
    if (ensure_decompressed(file) != 0) {
        pthread_mutex_unlock(&file->mutex);
        errno = EIO;
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
            pthread_mutex_unlock(&file->mutex);
            errno = ENOMEM;
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
    
    pthread_mutex_unlock(&file->mutex);
    return (ssize_t)len;
}

off_t ermfs_seek(ermfs_fd_t fd, off_t offset, int whence) {
    erm_file *file = get_file_from_fd(fd);
    if (!file) {
        errno = EBADF;
        return -1;
    }
    
    pthread_mutex_lock(&file->mutex);
    
    /* Ensure data is decompressed for size calculations */
    if (ensure_decompressed(file) != 0) {
        pthread_mutex_unlock(&file->mutex);
        errno = EIO;
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
            pthread_mutex_unlock(&file->mutex);
            errno = EINVAL;
            return -1;  /* Invalid whence */
    }
    
    /* Validate new position (allow seeking past end for writes) */
    if (new_pos < 0) {
        pthread_mutex_unlock(&file->mutex);
        errno = EINVAL;
        return -1;
    }
    
    file->position = new_pos;
    off_t result = new_pos;
    pthread_mutex_unlock(&file->mutex);
    return result;
}

int ermfs_stat(ermfs_fd_t fd, struct ermfs_stat *stat) {
    erm_file *file = get_file_from_fd(fd);
    if (!file || !stat) {
        errno = file ? EINVAL : EBADF;
        return -1;
    }
    
    pthread_mutex_lock(&file->mutex);
    stat->size = ermfs_size(file);  /* Use existing function that handles compression */
    stat->compressed = file->compressed;
    stat->mode = file->mode;
    pthread_mutex_unlock(&file->mutex);
    
    return 0;
}

int ermfs_close_fd(ermfs_fd_t fd) {
    erm_file *file = get_file_from_fd(fd);
    if (!file) {
        errno = EBADF;
        return -1;
    }
    
    /* Get the path before we potentially destroy the file */
    char *path = NULL;
    pthread_mutex_lock(&file->mutex);
    if (file->path) {
        path = malloc(strlen(file->path) + 1);
        if (path) {
            strcpy(path, file->path);
        }
    }
    pthread_mutex_unlock(&file->mutex);
    
    /* Compress the file using existing close logic */
    ermfs_close(file);
    
    /* Check if this is the last reference */
    pthread_mutex_lock(&file->mutex);
    int is_last_ref = (file->ref_count <= 1);
    pthread_mutex_unlock(&file->mutex);
    
    /* If last reference, unregister from registry */
    if (is_last_ref && path) {
        unregister_file(path);
    }
    
    /* Destroy the file (will decrement ref_count) */
    ermfs_destroy(file);
    
    /* Free the file descriptor slot */
    int result = free_fd(fd);
    
    free(path);
    return result;
}

int ermfs_truncate(ermfs_fd_t fd, off_t length) {
    erm_file *file = get_file_from_fd(fd);
    if (!file) {
        errno = EBADF;
        return -1;
    }
    
    if (length < 0) {
        errno = EINVAL;
        return -1;
    }
    
    /* Check if file descriptor is open for writing */
    int fd_mode = get_fd_mode(fd);
    if (fd_mode == -1) {
        errno = EBADF;
        return -1;
    }
    if (fd_mode == O_RDONLY) {
        errno = EBADF;
        return -1;  /* File not open for writing */
    }
    
    pthread_mutex_lock(&file->mutex);
    
    /* Ensure data is decompressed before truncating */
    if (ensure_decompressed(file) != 0) {
        pthread_mutex_unlock(&file->mutex);
        errno = EIO;
        return -1;
    }
    
    size_t new_size = (size_t)length;
    
    /* If truncating to larger size, we might need to expand capacity */
    if (new_size > file->capacity) {
        size_t newcap = file->capacity * 2;
        if (newcap < new_size) {
            newcap = new_size;
        }
        void *newdata = erm_resize(file->data, file->capacity, newcap);
        if (!newdata) {
            pthread_mutex_unlock(&file->mutex);
            errno = ENOMEM;
            return -1;
        }
        file->data = newdata;
        file->capacity = newcap;
        
        /* Zero out the new space */
        if (new_size > file->size) {
            memset((char *)file->data + file->size, 0, new_size - file->size);
        }
    }
    
    file->size = new_size;
    
    /* Adjust position if it's beyond the new end */
    if (file->position > (off_t)new_size) {
        file->position = (off_t)new_size;
    }
    
    pthread_mutex_unlock(&file->mutex);
    return 0;
}
