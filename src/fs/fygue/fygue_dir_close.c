#include <stdlib.h>
#include <string.h>
#include "fygue.h"
#include "fat/fat.h"

int fygue_dir_close(struct fygue_descriptor *desc)
{
    struct fygue_fsinfo *fsinfo;

    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_DIR) {
        errno = EBADF;
        return -1;
    }
    if (fygue_mount(&fsinfo, true) != 0) {
        errno = EIO;
        return -1;
    }
    fygue_fat_dir_close(&(fsinfo->fat), &desc->resolve.dir.fat);
    if (desc->resolve.dir.dirent != NULL)
    {
        for (int i = 0 ; i < desc->resolve.dir.count ; i++) {
            if (desc->resolve.dir.dirent[i] != NULL)
                free(desc->resolve.dir.dirent[i]);
        }
        free(desc->resolve.dir.dirent);
    }
    memset(desc, 0x00, sizeof(struct fygue_descriptor));
    return 0;
}
