#include <stdlib.h>
#include <string.h>
#include "fygue.h"

/* fygue_file_read(): write directory (EISDIR) */
ssize_t fygue_file_read(
    struct fygue_descriptor *desc,
    void *buffer,
    size_t size
) {
    struct fygue_fsinfo *fsinfo;

    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_FILE) {
        errno = EBADF;
        return -1;
    }
    if (buffer == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (fygue_file_sync(desc) != 0) {
        errno = EIO;
        return -1;
    }
    if (fygue_mount(&fsinfo, true) != 0) {
        errno = EIO;
        return -1;
    }
    return fygue_fat_file_read(
        &(fsinfo->fat),
        &(desc->resolve.file.fat),
        buffer,
        size
    );
}
