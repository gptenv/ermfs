#ifndef ERM_ALLOC_H
#define ERM_ALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Allocate an anonymous memory region of given size. Returns pointer on success or NULL. */
void *erm_alloc(size_t initial_size);

/* Resize a previously allocated memory region. May return a new pointer. */
void *erm_resize(void *ptr, size_t old_size, size_t new_size);

/* Free a memory region allocated with erm_alloc. */
void erm_free(void *ptr, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* ERM_ALLOC_H */
