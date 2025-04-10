#include <gint/hardware.h>
#include "fygue.h"
#include "fat/fat.h"

/* fygue_stat(): stat primitive */
int fygue_stat(char const * const path, struct stat *statbuf)
{
    struct fygue_descriptor desc;
    struct fygue_fsinfo *fsinfo;

    if(gint[HWFS] != HWFS_FUGUE)
       FYGUE_RET_ERRNO(ENOTSUP);
    if (path == NULL || statbuf == NULL)
       FYGUE_RET_ERRNO(EFAULT);
    if (fygue_mount(&fsinfo) != 0)
       FYGUE_RET_ERRNO(EIO);
    if (fygue_open_resolve(path, &desc) != 0)
       FYGUE_RET_ERRNO(ENOENT);
    if (desc.type == FYGUE_DESC_TYPE_DIR)
        return fygue_fat_dir_stat(&(fsinfo->fat), &(desc.dir.fat), statbuf);
    return fygue_fat_file_stat(&(fsinfo->fat), &(desc.file.fat), statbuf);
}
