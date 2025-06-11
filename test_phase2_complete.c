#include "ermfs/ermfs.h"
#include "ermfs/ermfd.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

void test_basic_export() {
    printf("Test 1: Basic memfd export...\n");
    
    const char *path = "/phase2/basic.txt";
    const char *msg = "Basic export test data";
    
    /* Create and write file */
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    
    /* Export before closing */
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);
    
    /* Verify content */
    char buf[64];
    ssize_t r = read(memfd, buf, sizeof(buf) - 1);
    assert(r == (ssize_t)strlen(msg));
    buf[r] = '\0';
    assert(strcmp(buf, msg) == 0);
    
    ermfs_close_fd(fd);
    close(memfd);
    printf("  Basic export test passed!\n");
}

void test_compressed_export() {
    printf("Test 2: Compressed file export...\n");
    
    const char *path = "/phase2/compressed.txt";
    const char *pattern = "Compressible repetitive data pattern. ";
    size_t pattern_len = strlen(pattern);
    
    /* Create file with compressible data */
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    
    for (int i = 0; i < 50; i++) {
        assert(ermfs_write_fd(fd, pattern, pattern_len) == (ssize_t)pattern_len);
    }
    
    /* Close to trigger compression */
    assert(ermfs_close_fd(fd) == 0);
    
    /* Export compressed file */
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);
    
    /* Verify size and content */
    struct stat st;
    assert(fstat(memfd, &st) == 0);
    size_t expected_size = pattern_len * 50;
    assert(st.st_size == (off_t)expected_size);
    
    /* Verify content integrity */
    char *buffer = malloc(expected_size);
    assert(buffer != NULL);
    lseek(memfd, 0, SEEK_SET);
    assert(read(memfd, buffer, expected_size) == (ssize_t)expected_size);
    
    for (int i = 0; i < 50; i++) {
        assert(memcmp(buffer + i * pattern_len, pattern, pattern_len) == 0);
    }
    
    free(buffer);
    close(memfd);
    printf("  Compressed export test passed!\n");
}

void test_mmap_compatibility() {
    printf("Test 3: mmap compatibility...\n");
    
    const char *path = "/phase2/mmap.txt";
    const char *msg = "mmap test data for memfd compatibility";
    
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    assert(ermfs_close_fd(fd) == 0);
    
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);
    
    /* Test mmap */
    void *map = mmap(NULL, strlen(msg), PROT_READ, MAP_PRIVATE, memfd, 0);
    assert(map != MAP_FAILED);
    assert(memcmp(map, msg, strlen(msg)) == 0);
    munmap(map, strlen(msg));
    
    close(memfd);
    printf("  mmap compatibility test passed!\n");
}

void test_fstat_compatibility() {
    printf("Test 4: fstat compatibility...\n");
    
    const char *path = "/phase2/fstat.txt";
    const char *msg = "fstat test data";
    
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    assert(ermfs_close_fd(fd) == 0);
    
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);
    
    /* Test fstat */
    struct stat st;
    assert(fstat(memfd, &st) == 0);
    assert(st.st_size == (off_t)strlen(msg));
    assert(S_ISREG(st.st_mode));
    
    close(memfd);
    printf("  fstat compatibility test passed!\n");
}

void test_lseek_compatibility() {
    printf("Test 5: lseek compatibility...\n");
    
    const char *path = "/phase2/seek.txt";
    const char *msg = "0123456789ABCDEF";
    
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    assert(ermfs_close_fd(fd) == 0);
    
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);
    
    /* Test seeking */
    assert(lseek(memfd, 5, SEEK_SET) == 5);
    char buf[5];
    assert(read(memfd, buf, 4) == 4);
    buf[4] = '\0';
    assert(strcmp(buf, "5678") == 0);
    
    assert(lseek(memfd, -2, SEEK_END) == (off_t)(strlen(msg) - 2));
    assert(read(memfd, buf, 2) == 2);
    buf[2] = '\0';
    assert(strcmp(buf, "EF") == 0);
    
    close(memfd);
    printf("  lseek compatibility test passed!\n");
}

void test_error_conditions() {
    printf("Test 6: Error conditions...\n");
    
    /* Test non-existent file */
    int bad_fd = ermfs_export_memfd("/nonexistent/path", 0);
    assert(bad_fd == -1);
    assert(errno == ENOENT);
    
    /* Test NULL path */
    bad_fd = ermfs_export_memfd(NULL, 0);
    assert(bad_fd == -1);
    assert(errno == EINVAL);
    
    printf("  Error conditions test passed!\n");
}

void test_multiple_exports() {
    printf("Test 7: Multiple exports of same file...\n");
    
    const char *path = "/phase2/multi.txt";
    const char *msg = "Multiple export test";
    
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    assert(ermfs_close_fd(fd) == 0);
    
    /* Export multiple times */
    int memfd1 = ermfs_export_memfd(path, 0);
    int memfd2 = ermfs_export_memfd(path, 0);
    assert(memfd1 >= 0 && memfd2 >= 0 && memfd1 != memfd2);
    
    /* Verify both exports are identical */
    char buf1[64], buf2[64];
    assert(read(memfd1, buf1, sizeof(buf1) - 1) == (ssize_t)strlen(msg));
    assert(read(memfd2, buf2, sizeof(buf2) - 1) == (ssize_t)strlen(msg));
    buf1[strlen(msg)] = buf2[strlen(msg)] = '\0';
    assert(strcmp(buf1, buf2) == 0);
    assert(strcmp(buf1, msg) == 0);
    
    close(memfd1);
    close(memfd2);
    printf("  Multiple exports test passed!\n");
}

void test_snapshot_semantics() {
    printf("Test 8: Snapshot semantics (export of open file)...\n");
    
    const char *path = "/phase2/snapshot.txt";
    const char *initial = "Initial data";
    const char *additional = " + Additional data";
    
    /* Create file and write initial data */
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    assert(ermfs_write_fd(fd, initial, strlen(initial)) == (ssize_t)strlen(initial));
    
    /* Export while file is still open */
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);
    
    /* Write more data to original file */
    assert(ermfs_write_fd(fd, additional, strlen(additional)) == (ssize_t)strlen(additional));
    
    /* Verify exported file only contains initial data (snapshot) */
    char buf[64];
    ssize_t r = read(memfd, buf, sizeof(buf) - 1);
    assert(r == (ssize_t)strlen(initial));
    buf[r] = '\0';
    assert(strcmp(buf, initial) == 0);
    
    ermfs_close_fd(fd);
    close(memfd);
    printf("  Snapshot semantics test passed!\n");
}

int main() {
    printf("Testing ERMFS Phase 2 Complete Implementation...\n\n");
    
    test_basic_export();
    test_compressed_export();
    test_mmap_compatibility();
    test_fstat_compatibility();
    test_lseek_compatibility();
    test_error_conditions();
    test_multiple_exports();
    test_snapshot_semantics();
    
    printf("\n🎉 All Phase 2 tests passed! ERMFS Phase 2 is COMPLETE! 🎉\n");
    return 0;
}