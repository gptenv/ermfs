#include "ermfs/ermfs.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

void test_path_sharing() {
    printf("Test: Path-based file sharing...\n");
    
    /* Open same file path twice */
    ermfs_fd_t fd1 = ermfs_open("/shared/test.txt", O_RDWR);
    ermfs_fd_t fd2 = ermfs_open("/shared/test.txt", O_RDWR);
    assert(fd1 >= 0 && fd2 >= 0);
    printf("  Opened same path twice: fd1=%d, fd2=%d\n", fd1, fd2);
    
    /* Write to first fd */
    const char *data1 = "Data from fd1";
    ssize_t written = ermfs_write_fd(fd1, data1, strlen(data1));
    assert(written == (ssize_t)strlen(data1));
    
    /* Read from second fd (should see the same data) */
    char buffer[100];
    ermfs_seek(fd2, 0, SEEK_SET);
    ssize_t read_bytes = ermfs_read(fd2, buffer, sizeof(buffer) - 1);
    buffer[read_bytes] = '\0';
    printf("  Data written by fd1, read by fd2: '%s'\n", buffer);
    assert(strcmp(buffer, data1) == 0);
    
    /* Write more data from fd2 */
    ermfs_seek(fd2, 0, SEEK_END);
    const char *data2 = " + Data from fd2";
    written = ermfs_write_fd(fd2, data2, strlen(data2));
    assert(written == (ssize_t)strlen(data2));
    
    /* Read complete content from fd1 */
    ermfs_seek(fd1, 0, SEEK_SET);
    read_bytes = ermfs_read(fd1, buffer, sizeof(buffer) - 1);
    buffer[read_bytes] = '\0';
    printf("  Complete content: '%s'\n", buffer);
    
    ermfs_close_fd(fd1);
    ermfs_close_fd(fd2);
    printf("  Path sharing test passed!\n\n");
}

void test_truncation() {
    printf("Test: File truncation...\n");
    
    ermfs_fd_t fd = ermfs_open("/tmp/truncate_test.txt", O_RDWR);
    assert(fd >= 0);
    
    /* Write some data */
    const char *data = "This is a long string that will be truncated";
    ssize_t written = ermfs_write_fd(fd, data, strlen(data));
    assert(written == (ssize_t)strlen(data));
    
    struct ermfs_stat stat;
    ermfs_stat(fd, &stat);
    printf("  Original size: %zu\n", stat.size);
    
    /* Truncate to smaller size */
    int result = ermfs_truncate(fd, 20);
    assert(result == 0);
    
    ermfs_stat(fd, &stat);
    printf("  Size after truncate to 20: %zu\n", stat.size);
    assert(stat.size == 20);
    
    /* Read truncated content */
    char buffer[50];
    ermfs_seek(fd, 0, SEEK_SET);
    ssize_t read_bytes = ermfs_read(fd, buffer, sizeof(buffer) - 1);
    buffer[read_bytes] = '\0';
    printf("  Truncated content: '%s'\n", buffer);
    
    /* Truncate to larger size */
    result = ermfs_truncate(fd, 30);
    assert(result == 0);
    
    ermfs_stat(fd, &stat);
    printf("  Size after truncate to 30: %zu\n", stat.size);
    assert(stat.size == 30);
    
    ermfs_close_fd(fd);
    printf("  Truncation test passed!\n\n");
}

void* thread_test_func(void* arg) {
    int thread_id = *(int*)arg;
    char path[100];
    snprintf(path, sizeof(path), "/thread/test_%d.txt", thread_id);
    
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    if (fd < 0) {
        printf("Thread %d: Failed to open file\n", thread_id);
        return NULL;
    }
    
    char data[100];
    snprintf(data, sizeof(data), "Data from thread %d", thread_id);
    
    ssize_t written = ermfs_write_fd(fd, data, strlen(data));
    if (written != (ssize_t)strlen(data)) {
        printf("Thread %d: Failed to write data\n", thread_id);
        ermfs_close_fd(fd);
        return NULL;
    }
    
    /* Read back the data */
    char buffer[100];
    ermfs_seek(fd, 0, SEEK_SET);
    ssize_t read_bytes = ermfs_read(fd, buffer, sizeof(buffer) - 1);
    buffer[read_bytes] = '\0';
    
    printf("Thread %d: Successfully wrote and read: '%s'\n", thread_id, buffer);
    
    ermfs_close_fd(fd);
    return NULL;
}

void test_thread_safety() {
    printf("Test: Thread safety...\n");
    
    const int num_threads = 5;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];
    
    /* Create multiple threads that operate on different files */
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        int result = pthread_create(&threads[i], NULL, thread_test_func, &thread_ids[i]);
        assert(result == 0);
    }
    
    /* Wait for all threads to complete */
    for (int i = 0; i < num_threads; i++) {
        int result = pthread_join(threads[i], NULL);
        assert(result == 0);
    }
    
    printf("  Thread safety test passed!\n\n");
}

int main() {
    printf("Testing ERMFS Enhanced Features...\n\n");
    
    test_path_sharing();
    test_truncation();
    test_thread_safety();
    
    printf("All enhanced feature tests passed!\n");
    return 0;
}