#ifndef ERM_COMPRESS_H
#define ERM_COMPRESS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compress data using gzip compression.
 * Returns pointer to compressed data on success, NULL on failure.
 * compressed_size will contain the size of compressed data.
 * Caller is responsible for freeing the returned buffer.
 */
void *erm_compress(const void *data, size_t data_size, size_t *compressed_size);

/* Decompress gzip-compressed data.
 * Returns pointer to decompressed data on success, NULL on failure.
 * decompressed_size will contain the size of decompressed data.
 * Caller is responsible for freeing the returned buffer.
 */
void *erm_decompress(const void *compressed_data, size_t compressed_size, size_t *decompressed_size);

#ifdef __cplusplus
}
#endif

#endif /* ERM_COMPRESS_H */