#include "fygue.h"
#include "fat/fat.h"

/* fygue_stat(): stat primitive */
int fygue_stat(char const * const path, struct stat *statbuf)
{
    struct fygue_resolve resolve;
    struct fygue_fsinfo *fsinfo;

    ENOTSUP_IF_NOT_FYGUE(-1);
    if (path == NULL || statbuf == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (fygue_mount(&fsinfo, true) != 0) {
        errno = EIO;
        return -1;
    }

    if (fygue_resolve(path, &resolve) != 0) {
        errno = ENOENT;
        return -1;
    }
    if (resolve.type == FYGUE_FILE_TYPE_DIR) {
        fygue_fat_dir_stat(
            &(fsinfo->fat),
            &(resolve.dir.fat),
            statbuf
        );
    }
    return fygue_fat_file_stat(
        &(fsinfo->fat),
        &(resolve.file.fat),
        statbuf
    );
}
