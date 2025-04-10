#include <stdlib.h>
#include <string.h>
#include "fygue.h"

/* fygue_file_close(): close directory */
int fygue_file_close(
    struct fygue_fsinfo *fsinfo,
    struct fygue_file *file
) {
    if (fsinfo == NULL || file == NULL)
        FYGUE_RET_ERRNO(EIO);
    fygue_fat_file_close(&(fsinfo->fat), &(file->fat));
    if (file->path != NULL)
        free(file->path);
    memset(file, 0x00, sizeof(struct fygue_file));
    return 0;
}
