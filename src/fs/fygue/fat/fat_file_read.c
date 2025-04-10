#include <errno.h>
#include <string.h>
#include "fat.h"

/* fygue_fat_file_read(): read primitive */
int fygue_fat_file_read(
    struct fygue_fat *fat,
    struct fygue_fat_file *file,
    void *buffer,
    size_t size
) {
    uintptr_t chunk_data_addr;
    size_t chunk_data_available;
    size_t chunk_data_offset;
    size_t read;

    if (fat == NULL || file == NULL || buffer == NULL)
        return -1;
    if (file->chunk == NULL)
        return -1;
    read = 0;
    chunk_data_offset = file->chunk_rd_offset;
    while (read < size)
    {
        if (file->chunk_rd_index >= file->chunk_count)
            break;
        chunk_data_addr  = file->chunk[file->chunk_rd_index].addr;
        chunk_data_addr += chunk_data_offset;
        chunk_data_available  = file->chunk[file->chunk_rd_index].size;
        chunk_data_available -= chunk_data_offset;
        if (read + chunk_data_available > size)
        {
            chunk_data_offset += size - read;
            memcpy(buffer, (void*)chunk_data_addr, size - read);
            read += size - read;
            break;
        }
        memcpy(buffer, (void*)chunk_data_addr, chunk_data_available);
        read += chunk_data_available;
        file->chunk_rd_index += 1;
        chunk_data_offset = 0;
    }
    file->chunk_rd_offset = chunk_data_offset;
    return read;
}
