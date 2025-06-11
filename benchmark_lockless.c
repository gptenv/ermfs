#include "ermfs/ermfs.h"
#include "ermfs/ermfs_lockless.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#define ITER 10000

static double bench(int lockless){
    ermfs_set_lockless_mode(lockless);
    struct timespec start,end;
    clock_gettime(CLOCK_MONOTONIC,&start);
    for(int i=0;i<ITER;i++){
        ermfs_fd_t fd=ermfs_open("/bench/file",O_RDWR);
        assert(fd>=0);
        ermfs_write_fd(fd,"a",1);
        ermfs_close_fd(fd);
    }
    clock_gettime(CLOCK_MONOTONIC,&end);
    return (end.tv_sec-start.tv_sec)+(end.tv_nsec-start.tv_nsec)/1e9;
}

int main(){
    double t1=bench(0);
    double t2=bench(1);
    printf("lock: %.3f s, lockless: %.3f s\n",t1,t2);
    return 0;
}
