#ifndef ERMFS_LOCKLESS_H
#define ERMFS_LOCKLESS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ermfs_set_lockless_mode(bool enable);
bool ermfs_is_lockless(void);

#ifdef __cplusplus
}
#endif

#endif /* ERMFS_LOCKLESS_H */
