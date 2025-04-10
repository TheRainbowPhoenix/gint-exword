#include <gint/hardware.h>
#include "fygue.h"

/* fygue_sync() - request filesystem re-synchronisation */
int fygue_sync(void)
{
    struct fygue_fsinfo *fsinfo;

    if(gint[HWFS] != HWFS_FUGUE)
       FYGUE_RET_ERRNO(ENOTSUP);
    if (fygue_mount(&fsinfo) != 0)
        FYGUE_RET_ERRNO(EIO);
    fsinfo->dirty = true;
    return 0;
}
