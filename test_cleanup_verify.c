#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    printf("Testing if files are truly cleaned up...\n");
    ermfs_set_lockless_mode(true);
    
    const char *path = "/cleanup_verify/test.txt";
    
    // First session: create file with specific content
    printf("First session: creating file...\n");
    ermfs_fd_t fd1 = ermfs_open(path, O_RDWR);
    if (fd1 < 0) {
        printf("Failed to open %s: %s\n", path, strerror(errno));
        return 1;
    }
    
    const char *msg1 = "first session data";
    if (ermfs_write_fd(fd1, msg1, strlen(msg1)) != (ssize_t)strlen(msg1)) {
        printf("Failed to write to first session\n");
        return 1;
    }
    
    printf("Closing first session...\n");
    if (ermfs_close_fd(fd1) != 0) {
        printf("Failed to close first session\n");
        return 1;
    }
    
    // Second session: reopen and check if it's a new file or contains old data
    printf("Second session: reopening file...\n");
    ermfs_fd_t fd2 = ermfs_open(path, O_RDWR);
    if (fd2 < 0) {
        printf("Failed to reopen %s: %s\n", path, strerror(errno));
        return 1;
    }
    
    // Try to read - if file was properly cleaned up, it should be empty
    char buf[64];
    ssize_t r = ermfs_read(fd2, buf, sizeof(buf)-1);
    buf[r] = '\0';
    
    if (r > 0) {
        printf("Found existing data: '%s' (length %zd)\n", buf, r);
        printf("This means file was NOT cleaned up from registry\n");
    } else {
        printf("File is empty (length %zd)\n", r);
        printf("This means file was properly cleaned up and this is a new file\n");
    }
    
    // Write new data to second session
    const char *msg2 = "second session data";
    if (ermfs_write_fd(fd2, msg2, strlen(msg2)) != (ssize_t)strlen(msg2)) {
        printf("Failed to write to second session\n");
        return 1;
    }
    
    printf("Closing second session...\n");
    if (ermfs_close_fd(fd2) != 0) {
        printf("Failed to close second session\n");
        return 1;
    }
    
    printf("Cleanup verification completed\n");
    return 0;
}