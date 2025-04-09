#include <string.h>
#include <stdlib.h>
#include "fat.h"

/* fygue_fat_file_open(): generate file information */
int fygue_fat_file_open(
    struct fygue_fat *fat,
    struct fygue_fat_file *file,
    struct fygue_fat_dirent *dirent
) {
    memset(file, 0x00, sizeof(struct fygue_fat_file));
    file->attribute             = dirent->attribute;
    file->cluster_entry         = dirent->cluster_id;
    file->size                  = dirent->size;
    file->chunk                 = NULL;
    file->chunk_count           = 0;
    file->chunk_table_max_index = 0;
    if (fygue_fat_file_sync(fat, file) != 0)
        return -1;
    if (fygue_fat_file_seek(fat, file, 0) != 0)
        return -2;
    return 0;
}


