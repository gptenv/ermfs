#ifndef ERMFD_H
#define ERMFD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Export an ERMFS-managed file as a memfd-backed descriptor */
int ermfs_export_memfd(const char *path, int flags);

#ifdef __cplusplus
}
#endif

#endif /* ERMFD_H */
