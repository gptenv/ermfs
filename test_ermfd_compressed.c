#include "ermfs/ermfs.h"
#include "ermfs/ermfd.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int main() {
    printf("Testing ERMFD with compressed files...\n");

    const char *path = "/memfd/compressed.txt";
    
    /* Create file with substantial data for compression */
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    
    /* Write repetitive data that should compress well */
    const char *pattern = "This is a repetitive pattern that should compress well. ";
    size_t pattern_len = strlen(pattern);
    
    for (int i = 0; i < 100; i++) {
        ssize_t written = ermfs_write_fd(fd, pattern, pattern_len);
        assert(written == (ssize_t)pattern_len);
    }
    
    /* Close file to trigger compression */
    printf("Closing file to trigger compression...\n");
    assert(ermfs_close_fd(fd) == 0);
    
    /* Now export the compressed file */
    printf("Exporting compressed file to memfd...\n");
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);
    
    /* Verify size matches original uncompressed size */
    struct stat st;
    assert(fstat(memfd, &st) == 0);
    size_t expected_size = pattern_len * 100;
    printf("Expected size: %zu, Actual size: %ld\n", expected_size, st.st_size);
    assert(st.st_size == (off_t)expected_size);
    
    /* Verify we can read the full content */
    char *buffer = malloc(expected_size + 1);
    assert(buffer != NULL);
    
    lseek(memfd, 0, SEEK_SET);
    ssize_t total_read = 0;
    while (total_read < (ssize_t)expected_size) {
        ssize_t r = read(memfd, buffer + total_read, expected_size - total_read);
        assert(r > 0);
        total_read += r;
    }
    buffer[expected_size] = '\0';
    
    /* Verify content integrity */
    for (int i = 0; i < 100; i++) {
        if (memcmp(buffer + i * pattern_len, pattern, pattern_len) != 0) {
            printf("Content mismatch at repetition %d\n", i);
            assert(0);
        }
    }
    
    /* Test mmap on compressed->decompressed content */
    void *map = mmap(NULL, expected_size, PROT_READ, MAP_PRIVATE, memfd, 0);
    assert(map != MAP_FAILED);
    assert(memcmp(map, buffer, expected_size) == 0);
    munmap(map, expected_size);
    
    free(buffer);
    close(memfd);
    
    printf("ERMFD compressed file export test passed!\n");
    return 0;
}
