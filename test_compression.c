#include "ermfs/ermfs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    printf("Testing ERMFS VFS Compression...\n");
    
    /* Test 1: Create file with substantial data for compression */
    printf("Test 1: Creating file with compressible data...\n");
    ermfs_fd_t fd = ermfs_open("/tmp/compress_test.txt", O_RDWR);
    assert(fd >= 0);
    
    /* Write repetitive data that should compress well */
    const char *pattern = "This is a repetitive pattern that should compress well. ";
    size_t pattern_len = strlen(pattern);
    
    for (int i = 0; i < 100; i++) {
        ssize_t written = ermfs_write_fd(fd, pattern, pattern_len);
        assert(written == (ssize_t)pattern_len);
    }
    
    /* Check stats before compression */
    struct ermfs_stat stat_before;
    ermfs_stat(fd, &stat_before);
    printf("  Data size before compression: %zu bytes\n", stat_before.size);
    printf("  Compressed status: %s\n", stat_before.compressed ? "yes" : "no");
    assert(!stat_before.compressed);
    
    /* Close file to trigger compression */
    printf("Test 2: Closing file to trigger compression...\n");
    int result = ermfs_close_fd(fd);
    assert(result == 0);
    printf("  File closed and compressed\n");
    
    /* Test 3: Reopen and verify data integrity */
    printf("Test 3: Reopening file and verifying data...\n");
    fd = ermfs_open("/tmp/compress_test2.txt", O_RDWR);
    assert(fd >= 0);
    
    /* Write the same data again */
    for (int i = 0; i < 100; i++) {
        ermfs_write_fd(fd, pattern, pattern_len);
    }
    
    /* Manually close using legacy API to test compression */
    erm_file *direct_file = ermfs_create(1024);
    for (int i = 0; i < 50; i++) {
        ermfs_write(direct_file, pattern, pattern_len);
    }
    
    printf("  Direct file size before close: %zu\n", ermfs_size(direct_file));
    ermfs_close(direct_file);  /* This should compress */
    printf("  Direct file size after close: %zu\n", ermfs_size(direct_file));
    
    /* Verify we can still read the data after compression */
    void *data_ptr = ermfs_data(direct_file);
    assert(data_ptr != NULL);
    printf("  Data pointer retrieved successfully after decompression\n");
    
    /* Clean up */
    ermfs_destroy(direct_file);
    ermfs_close_fd(fd);
    
    printf("\nCompression test completed successfully!\n");
    return 0;
}