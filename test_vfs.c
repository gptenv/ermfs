#include "ermfs/ermfs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    printf("Testing ERMFS VFS API...\n");
    
    /* Test 1: Open a file for writing */
    printf("Test 1: Opening file for writing...\n");
    ermfs_fd_t fd = ermfs_open("/tmp/test.txt", O_RDWR);
    assert(fd >= 0);
    printf("  File descriptor: %d\n", fd);
    
    /* Test 2: Write some data */
    printf("Test 2: Writing data...\n");
    const char *data = "Hello, ERMFS World!";
    ssize_t written = ermfs_write_fd(fd, data, strlen(data));
    assert(written == (ssize_t)strlen(data));
    printf("  Wrote %zd bytes\n", written);
    
    /* Test 3: Seek to beginning */
    printf("Test 3: Seeking to beginning...\n");
    off_t pos = ermfs_seek(fd, 0, SEEK_SET);
    assert(pos == 0);
    printf("  Current position: %ld\n", pos);
    
    /* Test 4: Read the data back */
    printf("Test 4: Reading data back...\n");
    char buffer[100];
    ssize_t read_bytes = ermfs_read(fd, buffer, sizeof(buffer) - 1);
    assert(read_bytes > 0);
    buffer[read_bytes] = '\0';
    printf("  Read %zd bytes: '%s'\n", read_bytes, buffer);
    assert(strcmp(buffer, data) == 0);
    
    /* Test 5: Get file statistics */
    printf("Test 5: Getting file statistics...\n");
    struct ermfs_stat stat;
    int result = ermfs_stat(fd, &stat);
    assert(result == 0);
    printf("  File size: %zu\n", stat.size);
    printf("  Compressed: %s\n", stat.compressed ? "yes" : "no");
    printf("  Mode: %d\n", stat.mode);
    
    /* Test 6: Seek to end and write more data */
    printf("Test 6: Seeking to end and writing more...\n");
    pos = ermfs_seek(fd, 0, SEEK_END);
    printf("  Position at end: %ld\n", pos);
    const char *more_data = " Additional text.";
    written = ermfs_write_fd(fd, more_data, strlen(more_data));
    assert(written == (ssize_t)strlen(more_data));
    
    /* Test 7: Read all data from beginning */
    printf("Test 7: Reading all data from beginning...\n");
    ermfs_seek(fd, 0, SEEK_SET);
    read_bytes = ermfs_read(fd, buffer, sizeof(buffer) - 1);
    buffer[read_bytes] = '\0';
    printf("  Complete content: '%s'\n", buffer);
    
    /* Test 8: Close the file */
    printf("Test 8: Closing file...\n");
    result = ermfs_close_fd(fd);
    assert(result == 0);
    printf("  File closed successfully\n");
    
    printf("\nAll tests passed! ERMFS VFS implementation working.\n");
    return 0;
}