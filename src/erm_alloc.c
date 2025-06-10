#define _GNU_SOURCE
#include "ermfs/erm_alloc.h"

#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void *erm_alloc(size_t initial_size) {
    if (initial_size == 0) {
        initial_size = getpagesize();
    }
    void *ptr = mmap(NULL, initial_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    return ptr;
}

void *erm_resize(void *ptr, size_t old_size, size_t new_size) {
    if (!ptr) {
        return erm_alloc(new_size);
    }
    void *new_ptr = mremap(ptr, old_size, new_size, MREMAP_MAYMOVE);
    if (new_ptr == MAP_FAILED) {
        return NULL;
    }
    return new_ptr;
}

void erm_free(void *ptr, size_t size) {
    if (!ptr || size == 0) {
        return;
    }
    munmap(ptr, size);
}
