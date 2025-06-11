#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

int main() {
    printf("Lockless correctness test\n");
    ermfs_set_lockless_mode(true);
    ermfs_fd_t fd = ermfs_open("/lockless/file.txt", O_RDWR);
    assert(fd >= 0);
    const char *msg = "lockless works";
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    ermfs_seek(fd, 0, SEEK_SET);
    char buf[64];
    ssize_t r = ermfs_read(fd, buf, sizeof(buf)-1);
    buf[r] = '\0';
    assert(strcmp(buf, msg) == 0);
    assert(ermfs_close_fd(fd) == 0);
    printf("Lockless correctness passed\n");
    return 0;
}
