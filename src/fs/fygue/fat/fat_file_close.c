#include <string.h>
#include <stdlib.h>
#include "fat.h"

/* fygue_fat_file_close(): close primitive */
int fygue_fat_file_close(struct fygue_fat *fat, struct fygue_fat_file *file)
{
    if (fat == NULL || file == NULL)
        return -1;
    if (file->chunk != NULL)
        free(file->chunk);
    memset(file, 0x00, sizeof(struct fygue_fat_file));
    return 0;
}
