#include "fygue.h"

/* fygue_dir_lseek(): seek directory */
off_t fygue_dir_lseek(
    struct fygue_fsinfo *fsinfo,
    struct fygue_dir *dir,
    off_t offset,
    int whence
) {
    if (fsinfo == NULL || dir == NULL)
        FYGUE_RET_ERRNO(EIO);

    if (whence != SEEK_CUR && whence != SEEK_SET && whence != SEEK_END)
        FYGUE_RET_ERRNO(EINVAL);
    if(whence == SEEK_CUR)
        offset += dir->pos;
    if(whence == SEEK_END)
        offset += dir->count;
    if(offset < 0 || offset >= dir->count + 1)
        FYGUE_RET_ERRNO(EINVAL);

    dir->pos = offset;
    return dir->pos;
}
