#include "ermfs/ermfs_lockless.h"

static bool lockless_enabled = false;

void ermfs_set_lockless_mode(bool enable) {
    lockless_enabled = enable;
}

bool ermfs_is_lockless(void) {
    return lockless_enabled;
}
