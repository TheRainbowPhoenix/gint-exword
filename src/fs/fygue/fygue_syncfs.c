#include "fygue.h"

int fygue_syncfs(void *data)
{
    struct fygue_fsinfo *fsinfo;
    struct fygue_file_descriptor *descriptor;

    if (data == NULL)
        return -1;
    if (fygue_mount(&fsinfo, false) != 0)
        return -2;
    descriptor = data;
    descriptor->dirty = true;
    fsinfo->dirty = true;
    return 0;
}
