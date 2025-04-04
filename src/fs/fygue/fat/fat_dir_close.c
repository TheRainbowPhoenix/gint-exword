#include "fat.h"

/* fygue_fat_dir_close(): closedir primitive */
int fygue_fat_dir_close(struct fygue_fat *fat, struct fygue_fat_dir *dir)
{
    if (fat == NULL || dir == NULL)
        return -1;
    // nothing to do here...
    return 0;
}

