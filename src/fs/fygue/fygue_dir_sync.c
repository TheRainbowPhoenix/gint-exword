#include <stdlib.h>
#include <gint/hardware.h>
#include "fygue.h"

/* fygue_dir_sync(): directory-specific sync */
int fygue_dir_sync(
    struct fygue_fsinfo *fsinfo,
    struct fygue_dir *dir
) {
    struct fygue_fat_descriptor desc;
    int saved_pos;
    int rc;

    /* preliminary check */
    if(gint[HWFS] != HWFS_FUGUE)
       FYGUE_RET_ERRNO(ENOTSUP);
    if (fsinfo == NULL || dir == NULL)
        FYGUE_RET_ERRNO(EIO);

    /* update FAT-specific information */
    //TODO: assume that the first cluster will remain the same ?
    if (fygue_fat_open(&(fsinfo->fat), dir->path, &desc) != 0)
        FYGUE_RET_ERRNO(ENOENT);
    if (desc.type != FYGUE_FAT_DESC_TYPE_FILE)
        FYGUE_RET_ERRNO(EIO);
    rc = fygue_fat_dir_sync(
        &(fsinfo->fat),
        &(desc.dir),
        desc.file.cluster_entry
    );
    if (rc != 0)
        FYGUE_RET_ERRNO(EIO);

    /* refresh internal information */
    //TODO: re-use realloc
    if (dir->dirent != NULL)
    {
        for (int i = 0 ; i < dir->count ; i++)
        {
            if (dir->dirent[i] == NULL)
                continue;
            free(dir->dirent[i]);
            dir->dirent[i] = NULL;
        }
        free(dir->dirent);
        dir->dirent = NULL;
        dir->count = 0;
    }
    /* hack to force regenerate the dirent cache */
    saved_pos = dir->pos;
    if (
        fygue_dir_lseek(fsinfo, dir, 0, SEEK_SET) != 0 ||
        fygue_dir_lseek(fsinfo, dir, saved_pos, SEEK_SET) != 0
    ) {
        return -1;
    }
    return 0;
}
