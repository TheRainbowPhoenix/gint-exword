#include <unistd.h>
#include <gint/hardware.h>
#include "fygue/fygue.h"

void sync(void)
{
    struct fygue_fsinfo *fsinfo;

    if(gint[HWFS] != HWFS_FUGUE)
        return;
    if (fygue_mount(&fsinfo) != 0)
        return;
    fsinfo->dirty = true;
    fygue_mount(&fsinfo);
}
