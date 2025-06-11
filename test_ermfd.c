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
    printf("Testing ERMFD memfd export...\n");

    const char *path = "/memfd/export.txt";
    const char *msg = "Hello from memfd";

    /* Create and write file */
    ermfs_fd_t fd = ermfs_open(path, O_RDWR);
    assert(fd >= 0);
    assert(ermfs_write_fd(fd, msg, strlen(msg)) == (ssize_t)strlen(msg));

    /* Export while file is still open */
    int memfd = ermfs_export_memfd(path, 0);
    assert(memfd >= 0);

    /* Now close the original file */
    assert(ermfs_close_fd(fd) == 0);

    /* Read back */
    char buf[64];
    ssize_t r = read(memfd, buf, sizeof(buf) - 1);
    assert(r == (ssize_t)strlen(msg));
    buf[r] = '\0';
    assert(strcmp(buf, msg) == 0);

    /* mmap */
    lseek(memfd, 0, SEEK_SET);
    void *map = mmap(NULL, strlen(msg), PROT_READ, MAP_PRIVATE, memfd, 0);
    assert(map != MAP_FAILED);
    assert(memcmp(map, msg, strlen(msg)) == 0);
    munmap(map, strlen(msg));

    /* fstat */
    struct stat st;
    assert(fstat(memfd, &st) == 0);
    assert(st.st_size == (off_t)strlen(msg));

    close(memfd);

    /* Error condition */
    int bad = ermfs_export_memfd("/no/such/file", 0);
    assert(bad == -1);

    printf("ERMFD memfd export tests passed!\n");
    return 0;
}
