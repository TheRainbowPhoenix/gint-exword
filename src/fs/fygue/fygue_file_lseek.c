#include "fygue.h"

/* fygue_file_lseek(): seek file */
off_t fygue_file_lseek(
    struct fygue_descriptor *desc,
    off_t offset,
    int whence
) {
    struct fygue_fsinfo *fsinfo;

    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_FILE) {
        errno = EBADF;
        return -1;
    }
    if (whence != SEEK_CUR && whence != SEEK_SET && whence != SEEK_END) {
        errno = EINVAL;
        return -1;
    }
    if (fygue_descriptor_sync(&fsinfo, desc) != 0) {
        errno = EIO;
        return -1;
    }
    if(whence == SEEK_CUR)
        offset += desc->resolve.dir.pos;
    if(whence == SEEK_END)
        offset += desc->resolve.file.size;
    if(offset < 0 || offset >= desc->resolve.file.size + 1) {
        errno = EINVAL;
        return -1;
    }
    desc->resolve.file.cursor = offset;
    fygue_fat_file_seek(
        &(fsinfo->fat),
        &(desc->resolve.file.fat),
        offset
    );
    return desc->resolve.file.cursor;
}
