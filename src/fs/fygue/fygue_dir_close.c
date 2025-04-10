#include <stdlib.h>
#include <string.h>
#include <gint/hardware.h>
#include "fygue.h"
#include "fat/fat.h"

/* fygue_dir_close(): close directory */
int fygue_dir_close(
    struct fygue_fsinfo *fsinfo,
    struct fygue_dir *dir
) {
    if(gint[HWFS] != HWFS_FUGUE)
       FYGUE_RET_ERRNO(ENOTSUP);
    if (fsinfo == NULL || dir == NULL)
        FYGUE_RET_ERRNO(EIO);
    fygue_fat_dir_close(&(fsinfo->fat), &(dir->fat));
    if (dir->dirent != NULL)
    {
        for (int i = 0 ; i < dir->count ; i++) {
            if (dir->dirent[i] != NULL)
                free(dir->dirent[i]);
        }
        free(dir->dirent);
    }
    if (dir->path != NULL)
        free(dir->path);
    memset(dir, 0x00, sizeof(struct fygue_dir));
    return 0;
}
