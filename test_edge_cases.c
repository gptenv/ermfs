#include "ermfs/ermfs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    printf("Testing ERMFS VFS Edge Cases...\n");
    
    /* Test 1: Multiple file descriptors */
    printf("Test 1: Multiple file descriptors...\n");
    ermfs_fd_t fd1 = ermfs_open("/tmp/file1.txt", O_RDWR);
    ermfs_fd_t fd2 = ermfs_open("/tmp/file2.txt", O_RDWR);
    assert(fd1 >= 0 && fd2 >= 0 && fd1 != fd2);
    printf("  fd1: %d, fd2: %d\n", fd1, fd2);
    
    /* Write different data to each file */
    ermfs_write_fd(fd1, "File 1 content", 14);
    ermfs_write_fd(fd2, "File 2 content", 14);
    
    /* Read back and verify independence */
    char buf1[50], buf2[50];
    ermfs_seek(fd1, 0, SEEK_SET);
    ermfs_seek(fd2, 0, SEEK_SET);
    ermfs_read(fd1, buf1, sizeof(buf1) - 1);
    ermfs_read(fd2, buf2, sizeof(buf2) - 1);
    buf1[14] = buf2[14] = '\0';
    printf("  File 1: '%s'\n", buf1);
    printf("  File 2: '%s'\n", buf2);
    assert(strcmp(buf1, "File 1 content") == 0);
    assert(strcmp(buf2, "File 2 content") == 0);
    
    /* Test 2: Read-only mode */
    printf("Test 2: Read-only mode restrictions...\n");
    ermfs_fd_t fd_ro = ermfs_open("/tmp/readonly.txt", O_RDONLY);
    assert(fd_ro >= 0);
    ssize_t result = ermfs_write_fd(fd_ro, "test", 4);
    assert(result == -1);  /* Should fail */
    printf("  Write to read-only file correctly failed\n");
    
    /* Test 3: Write-only mode */
    printf("Test 3: Write-only mode restrictions...\n");
    ermfs_fd_t fd_wo = ermfs_open("/tmp/writeonly.txt", O_WRONLY);
    assert(fd_wo >= 0);
    ermfs_write_fd(fd_wo, "test data", 9);
    char read_buf[10];
    result = ermfs_read(fd_wo, read_buf, sizeof(read_buf));
    assert(result == -1);  /* Should fail */
    printf("  Read from write-only file correctly failed\n");
    
    /* Test 4: Seek beyond end and write */
    printf("Test 4: Seek beyond end and write...\n");
    ermfs_fd_t fd_seek = ermfs_open("/tmp/seektest.txt", O_RDWR);
    assert(fd_seek >= 0);
    ermfs_write_fd(fd_seek, "start", 5);
    off_t pos = ermfs_seek(fd_seek, 10, SEEK_SET);  /* Seek beyond end */
    assert(pos == 10);
    ermfs_write_fd(fd_seek, "end", 3);
    struct ermfs_stat stat;
    ermfs_stat(fd_seek, &stat);
    printf("  File size after seek-write: %zu\n", stat.size);
    assert(stat.size == 13);  /* 10 + 3 */
    
    /* Test 5: Error conditions */
    printf("Test 5: Error conditions...\n");
    result = ermfs_read(-1, read_buf, sizeof(read_buf));
    assert(result == -1);
    result = ermfs_write_fd(-1, "test", 4);
    assert(result == -1);
    result = ermfs_close_fd(-1);
    assert(result == -1);
    printf("  Invalid file descriptor operations correctly failed\n");
    
    /* Clean up */
    printf("Cleaning up...\n");
    ermfs_close_fd(fd1);
    ermfs_close_fd(fd2);
    ermfs_close_fd(fd_ro);
    ermfs_close_fd(fd_wo);
    ermfs_close_fd(fd_seek);
    
    printf("\nAll edge case tests passed!\n");
    return 0;
}