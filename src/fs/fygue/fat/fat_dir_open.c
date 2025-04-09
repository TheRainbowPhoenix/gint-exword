#include <string.h>
#include "fat.h"

/* fugue_fat_dir_open(): convert dirent to dir */
int fygue_fat_dir_open(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir,
    struct fygue_fat_dirent *dirent
) {
    int rc;

    memset(dir, 0x00, sizeof(struct fygue_fat_dir));
    if (dirent == NULL) {
        dir->cluster_entry      = 0;
        dir->root_dirent_count  = fat->RootEntCount;
        dir->attribute          = FYGUE_FAT_FILE_TYPE_DIR;
    } else {
        if ((dirent->attribute & FYGUE_FAT_FILE_TYPE_DIR) == 0x00)
            return -1;
        dir->cluster_entry      = dirent->cluster_id;
        dir->attribute          = FYGUE_FAT_FILE_TYPE_DIR;
        dir->root_dirent_count  = 0;
    }
    dir->dirty           = false;
    dir->sector_count    = 0;
    dir->dirent_cursor   = 0;
    dir->end_of_dirent   = false;
    dir->cluster_current = dir->cluster_entry;
    rc = fygue_fat_cluster_get_sector(
        fat,
        dir->cluster_current,
        &dir->sector_id
    );
    if (rc != 0)
        return -2;
    rc = fygue_fat_sector_get_addr(
        fat,
        &dir->dirent_current_addr,
        dir->sector_id
    );
    if (rc != 0)
        return -3;
    return 0;
}
