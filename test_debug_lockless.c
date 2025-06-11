#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    printf("Debug lockless operations...\n");
    ermfs_set_lockless_mode(true);
    
    // Test basic operation
    ermfs_fd_t fd = ermfs_open("/debug/test.txt", O_RDWR);
    if (fd < 0) {
        printf("Failed to open: %s\n", strerror(errno));
        return 1;
    }
    printf("Opened fd: %d\n", fd);
    
    const char *msg = "test data";
    ssize_t w = ermfs_write_fd(fd, msg, strlen(msg));
    if (w < 0) {
        printf("Failed to write: %s\n", strerror(errno));
        ermfs_close_fd(fd);
        return 1;
    }
    printf("Wrote %zd bytes\n", w);
    
    if (ermfs_seek(fd, 0, SEEK_SET) < 0) {
        printf("Failed to seek: %s\n", strerror(errno));
        ermfs_close_fd(fd);
        return 1;
    }
    
    char buf[64];
    ssize_t r = ermfs_read(fd, buf, sizeof(buf)-1);
    if (r < 0) {
        printf("Failed to read: %s\n", strerror(errno));
        ermfs_close_fd(fd);
        return 1;
    }
    buf[r] = '\0';
    printf("Read %zd bytes: '%s'\n", r, buf);
    
    if (strcmp(buf, msg) == 0) {
        printf("✅ Data matches!\n");
    } else {
        printf("❌ Data mismatch!\n");
    }
    
    ermfs_close_fd(fd);
    printf("✅ Basic debug test passed\n");
    
    // Test multiple opens to same file
    printf("\nTesting multiple FDs to same file...\n");
    ermfs_fd_t fd1 = ermfs_open("/debug/multi.txt", O_RDWR);
    ermfs_fd_t fd2 = ermfs_open("/debug/multi.txt", O_RDWR);
    
    if (fd1 < 0 || fd2 < 0) {
        printf("Failed to open multiple FDs: fd1=%d fd2=%d\n", fd1, fd2);
        return 1;
    }
    
    printf("Opened fd1=%d, fd2=%d\n", fd1, fd2);
    
    // Write to fd1
    if (ermfs_write_fd(fd1, "data1", 5) != 5) {
        printf("Failed to write to fd1\n");
        return 1;
    }
    
    // Write to fd2 
    if (ermfs_write_fd(fd2, "data2", 5) != 5) {
        printf("Failed to write to fd2\n"); 
        return 1;
    }
    
    // Seek both to beginning
    ermfs_seek(fd1, 0, SEEK_SET);
    ermfs_seek(fd2, 0, SEEK_SET);
    
    // Read from both
    char buf1[16], buf2[16];
    ssize_t r1 = ermfs_read(fd1, buf1, 15);
    ssize_t r2 = ermfs_read(fd2, buf2, 15);
    
    buf1[r1] = '\0';
    buf2[r2] = '\0';
    
    printf("fd1 read: '%s', fd2 read: '%s'\n", buf1, buf2);
    
    ermfs_close_fd(fd1);
    ermfs_close_fd(fd2);
    
    printf("✅ Multiple FD test completed\n");
    return 0;
}