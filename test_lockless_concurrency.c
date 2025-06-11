#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define THREADS 4

void* worker(void* arg) {
    int id = *(int*)arg;
    char path[64];
    snprintf(path, sizeof(path), "/lockless/thread_%d.txt", id);
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    char msg[32];
    snprintf(msg, sizeof(msg), "msg%d", id);
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));
    ermfs_seek(fd, 0, SEEK_SET);
    char buf[32];
    ssize_t r = ermfs_read(fd, buf, sizeof(buf)-1);
    buf[r] = '\0';
    assert(strcmp(buf, msg) == 0);
    ermfs_close_fd(fd);
    return NULL;
}

int main(){
    printf("Lockless concurrency test\n");
    ermfs_set_lockless_mode(true);
    pthread_t threads[THREADS];
    int ids[THREADS];
    for(int i=0;i<THREADS;i++){ids[i]=i;pthread_create(&threads[i],NULL,worker,&ids[i]);}
    for(int i=0;i<THREADS;i++) pthread_join(threads[i],NULL);
    printf("Lockless concurrency passed\n");
    return 0;
}
