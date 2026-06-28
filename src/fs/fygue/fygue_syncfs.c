#include "fygue.h"
#include "../fugue/fugue.h"

/* fygue_gfdi_syncfs(): flush buffers of the fs */
int fygue_syncfs(
    struct fygue_fsinfo *fsinfo,
    struct fygue_descriptor *desc
) {
    if (fsinfo == NULL || desc == NULL)
        FYGUE_RET_ERRNO(EIO);
    fugue_flush_all();
    return 0;
}
