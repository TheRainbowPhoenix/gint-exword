#include <errno.h>
#include "fat.h"

/* fygue_fat_file_seek(): seek primitive */
int fygue_fat_file_seek(
    struct fygue_fat *fat,
    struct fygue_fat_file *file,
    off_t offset
) {
    off_t offset_test;

    if (fat == NULL || file == NULL) {
        errno = EIO;
        return -1;
    }
    offset_test = 0;
    file->chunk_rd_index = 0;
    file->chunk_rd_offset = 0;
    for (int i = 0; i < file->chunk_count; i++)
    {
        offset_test += file->chunk[i].size;
        if (offset_test > offset) {
            file->chunk_rd_offset = offset - offset_test;
            return 0;
        }
        file->chunk_rd_index += 1;
    }
    errno = EIO;
    return -1;
}
