#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    printf("Testing resource cleanup...\n");
    ermfs_set_lockless_mode(true);
    
    // Test opening and closing many files
    for (int i = 0; i < 10; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/cleanup/file_%d.txt", i);
        
        ermfs_fd_t fd = ermfs_open(path, O_RDWR);
        if (fd < 0) {
            printf("Failed to open %s at iteration %d: %s\n", path, i, strerror(errno));
            return 1;
        }
        
        printf("Opened %s as fd %d\n", path, fd);
        
        // Write some data
        const char *msg = "test data";
        if (ermfs_write_fd(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) {
            printf("Failed to write to %s\n", path);
            ermfs_close_fd(fd);
            return 1;
        }
        
        // Close immediately
        if (ermfs_close_fd(fd) != 0) {
            printf("Failed to close %s\n", path);
            return 1;
        }
        
        printf("Closed %s\n", path);
    }
    
    printf("✅ Resource cleanup test completed\n");
    return 0;
}