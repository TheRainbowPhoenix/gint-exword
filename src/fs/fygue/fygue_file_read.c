#include "fygue.h"

/* fygue_file_read(): read file data */
ssize_t fygue_file_read(
    struct fygue_fsinfo *fsinfo,
    struct fygue_file *file,
    void *buffer,
    size_t size
) {
    if (fsinfo == NULL || file == NULL)
        FYGUE_RET_ERRNO(EIO);
    if (buffer == NULL)
        FYGUE_RET_ERRNO(EFAULT);
    return fygue_fat_file_read(
        &(fsinfo->fat),
        &(file->fat),
        buffer,
        size
    );
}
