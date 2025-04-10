#include <gint/hardware.h>
#include "fygue.h"

/* fygue_syncfs() - request filesystem re-synchronisation */
int fygue_syncfs(void *data)
{
    struct fygue_fsinfo *fsinfo;
    struct fygue_descriptor *desc;

    if(gint[HWFS] != HWFS_FUGUE)
       FYGUE_RET_ERRNO(ENOTSUP);
    if (data == NULL)
        FYGUE_RET_ERRNO(EBADF);
    if (fygue_mount(&fsinfo) != 0)
        FYGUE_RET_ERRNO(EIO);
    desc = data;
    desc->dirty = true;
    fsinfo->dirty = true;
    return 0;
}
