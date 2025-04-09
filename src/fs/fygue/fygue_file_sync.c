#include "fygue.h"

/* fygue_file_sync(): file-specific sync */
int fygue_file_sync(struct fygue_descriptor *desc)
{
    struct fygue_fsinfo *fsinfo;
    int rc;

    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_FILE) {
        errno = EBADF;
        return -1;
    }
    if (fygue_mount(&fsinfo, true)) {
        errno = EIO;
        return -1;
    }
    // nothing to sync in high-level
    rc = fygue_fat_file_sync(
        &(fsinfo->fat),
        &(desc->resolve.file.fat)
    );
    if (rc != 0)
        return -1;
    return 0;
}
