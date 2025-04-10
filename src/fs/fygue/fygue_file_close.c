#include <stdlib.h>
#include "fygue.h"
#include "fat/fat.h"

/* fygue_file_close(): close directory */
int fygue_file_close(
    struct fygue_fsinfo *fsinfo,
    struct fygue_file *file
) {
    if (fsinfo == NULL || file == NULL)
        FYGUE_RET_ERRNO(EIO);
    fygue_fat_file_close(&(fsinfo->fat), &(file->fat));
    free(file);
    return 0;
}
