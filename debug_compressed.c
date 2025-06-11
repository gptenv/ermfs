#include "ermfs/ermfs.h"
#include "ermfs/ermfd.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

int main() {
    printf("Debugging compressed file export...\n");

    const char *path = "/memfd/compressed.txt";
    
    /* Create file with substantial data for compression */
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    printf("Opened file: fd=%d\n", fd);
    
    /* Write repetitive data that should compress well */
    const char *pattern = "This is a repetitive pattern that should compress well. ";
    size_t pattern_len = strlen(pattern);
    
    for (int i = 0; i < 10; i++) {  // Smaller amount for testing
        ssize_t written = ermfs_write_fd(fd, pattern, pattern_len);
        assert(written == (ssize_t)pattern_len);
    }
    
    /* Check file before closing */
    struct ermfs_stat stat_before;
    int result = ermfs_stat(fd, &stat_before);
    printf("Before close - stat result: %d, size: %zu, compressed: %s\n", 
           result, stat_before.size, stat_before.compressed ? "yes" : "no");
    
    /* Close file to trigger compression */
    printf("Closing file to trigger compression...\n");
    result = ermfs_close_fd(fd);
    printf("Close result: %d\n", result);
    
    /* Try to export the compressed file */
    printf("Attempting to export compressed file to memfd...\n");
    int memfd = ermfs_export_memfd(path, 0);
    printf("Export result: %d, errno: %d (%s)\n", memfd, errno, strerror(errno));
    
    if (memfd >= 0) {
        printf("Export successful!\n");
        close(memfd);
    } else {
        printf("Export failed!\n");
    }
    
    return 0;
}
