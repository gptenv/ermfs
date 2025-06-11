#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define THREADS 8
#define ITERATIONS 100

struct thread_data {
    int thread_id;
    int iterations;
    int success_count;
};

void* stress_worker(void* arg) {
    struct thread_data *data = (struct thread_data*)arg;
    data->success_count = 0;
    
    for (int i = 0; i < data->iterations; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/stress/thread_%d_file_%d.txt", data->thread_id, i);
        
        // Open file
        ermfs_fd_t fd = ermfs_open(path, O_RDWR);
        if (fd < 0) {
            printf("Thread %d iter %d: Failed to open %s: %s\n", 
                   data->thread_id, i, path, strerror(errno));
            continue;
        }
        
        // Write some data
        char msg[64];
        snprintf(msg, sizeof(msg), "Thread %d iteration %d data", data->thread_id, i);
        if (ermfs_write_fd(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) {
            ermfs_close_fd(fd);
            continue;
        }
        
        // Seek to beginning
        if (ermfs_seek(fd, 0, SEEK_SET) != 0) {
            ermfs_close_fd(fd);
            continue;
        }
        
        // Read back data
        char buf[64];
        ssize_t r = ermfs_read(fd, buf, sizeof(buf)-1);
        if (r != (ssize_t)strlen(msg)) {
            ermfs_close_fd(fd);
            continue;
        }
        buf[r] = '\0';
        
        // Verify data
        if (strcmp(buf, msg) == 0) {
            data->success_count++;
        }
        
        ermfs_close_fd(fd);
    }
    
    return NULL;
}

int main() {
    printf("Lockless stress test (8 threads, 100 iterations each)...\n");
    ermfs_set_lockless_mode(true);
    
    pthread_t threads[THREADS];
    struct thread_data data[THREADS];
    
    // Start threads
    for (int i = 0; i < THREADS; i++) {
        data[i].thread_id = i;
        data[i].iterations = ITERATIONS;
        data[i].success_count = 0;
        pthread_create(&threads[i], NULL, stress_worker, &data[i]);
    }
    
    // Wait for all threads
    int total_success = 0;
    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_success += data[i].success_count;
        printf("Thread %d: %d/%d operations successful\n", 
               i, data[i].success_count, data[i].iterations);
    }
    
    printf("Total: %d/%d operations successful (%.1f%%)\n", 
           total_success, THREADS * ITERATIONS, 
           100.0 * total_success / (THREADS * ITERATIONS));
    
    if (total_success == THREADS * ITERATIONS) {
        printf("✅ Lockless stress test PASSED!\n");
        return 0;
    } else {
        printf("❌ Lockless stress test FAILED!\n");
        return 1;
    }
}