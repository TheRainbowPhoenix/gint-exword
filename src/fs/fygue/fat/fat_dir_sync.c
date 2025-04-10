#include "fat.h"

/* fygue_fat_dir_sync(): sync directory information */
int fygue_fat_dir_sync(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir,
    int cluster_id
) {
    int rc;

    if (fat == NULL || dir == NULL)
        return -1;
    dir->dirty           = false;
    dir->sector_count    = 0;
    dir->dirent_cursor   = 0;
    dir->end_of_dirent   = false;
    dir->cluster_entry   = cluster_id;
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
