#include "fygue.h"

/* fygue_fsync(): flush buffers of the file */
int fygue_fsync(
    struct fygue_fsinfo *fsinfo,
    struct fygue_descriptor *desc
) {
    if (fsinfo == NULL || desc == NULL)
        FYGUE_RET_ERRNO(EIO);
    return 0; // we don't support writing so nothing to flus
}
