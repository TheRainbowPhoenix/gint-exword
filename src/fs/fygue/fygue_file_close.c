#include <stdlib.h>
#include <string.h>
#include "fygue.h"
#include "fat/fat.h"

/* fygue_file_close(): close directory */
int fygue_file_close(struct fygue_descriptor *desc)
{
    struct fygue_fsinfo *fsinfo;

    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_FILE) {
        errno = EBADF;
        return -1;
    }
    if (fygue_descriptor_sync(&fsinfo, desc) != 0) {
        errno = EIO;
        return -1;
    }
    fygue_fat_file_close(&(fsinfo->fat), &desc->resolve.file.fat);
    memset(desc, 0x00, sizeof(struct fygue_descriptor));
    return 0;
}
