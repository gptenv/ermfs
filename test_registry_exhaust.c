#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    printf("Testing registry exhaustion...\n");
    ermfs_set_lockless_mode(true);
    
    // Try to open 300 files to see when it fails
    for (int i = 0; i < 300; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/exhaust/file_%d.txt", i);
        
        ermfs_fd_t fd = ermfs_open(path, O_RDWR);
        if (fd < 0) {
            printf("Failed to open file %d (%s): %s\n", i, path, strerror(errno));
            break;
        }
        
        // Write some data
        const char *msg = "data";
        if (ermfs_write_fd(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) {
            printf("Failed to write to file %d\n", i);
            ermfs_close_fd(fd);
            break;
        }
        
        // Close immediately
        if (ermfs_close_fd(fd) != 0) {
            printf("Failed to close file %d\n", i);
            break;
        }
        
        if (i % 10 == 9) {
            printf("Successfully handled %d files\n", i + 1);
        }
    }
    
    printf("Registry exhaustion test completed\n");
    return 0;
}