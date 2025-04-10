#include <string.h>
#include "fat.h"

/* fugue_fat_dir_open(): convert dirent to dir */
int fygue_fat_dir_open(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir,
    struct fygue_fat_dirent *dirent
) {
    memset(dir, 0x00, sizeof(struct fygue_fat_dir));
    if (dirent == NULL) {
        dir->cluster_entry      = 0;
        dir->root_dirent_count  = fat->RootEntCount;
        dir->attribute          = FYGUE_FAT_DESC_TYPE_DIR;
    } else {
        if ((dirent->attribute & FYGUE_FAT_ATTR_DIRECTORY) == 0x00)
            return -1;
        dir->cluster_entry      = dirent->cluster_id;
        dir->attribute          = FYGUE_FAT_DESC_TYPE_DIR;
        dir->root_dirent_count  = 0;
    }
    return fygue_fat_dir_sync(fat, dir, dir->cluster_entry);
}
