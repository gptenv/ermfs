#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    printf("Testing registry cleanup...\n");
    ermfs_set_lockless_mode(true);
    
    // Open and close a few files to see if cleanup works
    for (int i = 0; i < 5; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/cleanup_debug/file_%d.txt", i);
        
        printf("Opening %s...\n", path);
        ermfs_fd_t fd = ermfs_open(path, O_RDWR);
        if (fd < 0) {
            printf("Failed to open %s: %s\n", path, strerror(errno));
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
        printf("Wrote data to %s\n", path);
        
        // Close 
        printf("Closing %s...\n", path);
        if (ermfs_close_fd(fd) != 0) {
            printf("Failed to close %s\n", path);
            return 1;
        }
        printf("Closed %s\n", path);
        
        // Try to reopen the same file
        printf("Reopening %s...\n", path);
        ermfs_fd_t fd2 = ermfs_open(path, O_RDWR);
        if (fd2 < 0) {
            printf("Failed to reopen %s: %s\n", path, strerror(errno));
            return 1;
        }
        printf("Reopened %s as fd %d\n", path, fd2);
        
        // Close again
        if (ermfs_close_fd(fd2) != 0) {
            printf("Failed to close reopened %s\n", path);
            return 1;
        }
        printf("Closed reopened %s\n", path);
        
        printf("--- Iteration %d completed ---\n", i);
    }
    
    printf("Registry cleanup debug test completed\n");
    return 0;
}