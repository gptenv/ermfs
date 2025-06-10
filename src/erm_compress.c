#include "ermfs/erm_compress.h"
#include <zlib.h>
#include <stdlib.h>
#include <string.h>

void *erm_compress(const void *data, size_t data_size, size_t *compressed_size) {
    if (!data || data_size == 0 || !compressed_size) {
        return NULL;
    }

    /* Calculate maximum possible compressed size */
    uLong max_compressed_size = compressBound(data_size);
    void *compressed_buffer = malloc(max_compressed_size);
    if (!compressed_buffer) {
        return NULL;
    }

    /* Compress the data */
    uLong actual_compressed_size = max_compressed_size;
    int result = compress((Bytef *)compressed_buffer, &actual_compressed_size,
                         (const Bytef *)data, data_size);
    
    if (result != Z_OK) {
        free(compressed_buffer);
        return NULL;
    }

    /* Resize buffer to actual compressed size to save memory */
    void *final_buffer = realloc(compressed_buffer, actual_compressed_size);
    if (final_buffer) {
        compressed_buffer = final_buffer;
    }

    *compressed_size = actual_compressed_size;
    return compressed_buffer;
}

void *erm_decompress(const void *compressed_data, size_t compressed_size, size_t *decompressed_size) {
    if (!compressed_data || compressed_size == 0 || !decompressed_size) {
        return NULL;
    }

    /* Start with a reasonable buffer size and grow if needed */
    size_t buffer_size = compressed_size * 4; /* Initial guess: 4x expansion */
    void *buffer = malloc(buffer_size);
    if (!buffer) {
        return NULL;
    }

    uLong actual_size = buffer_size;
    int result = uncompress((Bytef *)buffer, &actual_size,
                           (const Bytef *)compressed_data, compressed_size);

    /* If buffer was too small, try with a larger buffer */
    while (result == Z_BUF_ERROR) {
        buffer_size *= 2;
        void *new_buffer = realloc(buffer, buffer_size);
        if (!new_buffer) {
            free(buffer);
            return NULL;
        }
        buffer = new_buffer;
        actual_size = buffer_size;
        result = uncompress((Bytef *)buffer, &actual_size,
                           (const Bytef *)compressed_data, compressed_size);
    }

    if (result != Z_OK) {
        free(buffer);
        return NULL;
    }

    /* Resize buffer to actual decompressed size */
    void *final_buffer = realloc(buffer, actual_size);
    if (final_buffer) {
        buffer = final_buffer;
    }

    *decompressed_size = actual_size;
    return buffer;
}