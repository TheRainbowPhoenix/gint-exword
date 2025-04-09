#include "fat.h"

/* fygue_fat_dir_sync(): sync directory information */
int fygue_fat_dir_sync(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir
) {
    if (fat == NULL || dir == NULL)
        return -1;
    // nothing to do (?)
    return 0;
}
