#include <stdlib.h>
#include "fygue.h"

/* fygue_dir_sync(): directory-specific sync */
int fygue_dir_sync(struct fygue_descriptor *desc)
{
    struct fygue_fsinfo *fsinfo;
    int rc;

    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_DIR) {
        errno = EBADF;
        return -1;
    }
    //if (desc->)
    if (fygue_mount(&fsinfo, true)) {
        errno = EIO;
        return -1;
    }
    for (int i = 0 ; i < desc->resolve.dir.count ; i++) {
        if (desc->resolve.dir.dirent[i] == NULL)
            continue;
        free(desc->resolve.dir.dirent[i]);
        desc->resolve.dir.dirent[i] = NULL;
    }
    rc = fygue_fat_dir_sync(
        &(fsinfo->fat),
        &(desc->resolve.dir.fat)
    );
    if (rc != 0)
        return -2;
    return 0;
}
