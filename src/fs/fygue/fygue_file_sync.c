#include <gint/hardware.h>
#include "fygue.h"

/* fygue_file_sync(): file-specific sync */
int fygue_file_sync(
    struct fygue_fsinfo *fsinfo,
    struct fygue_file *file
) {
    struct fygue_fat_descriptor desc;
    int rc;

    if(gint[HWFS] != HWFS_FUGUE)
       FYGUE_RET_ERRNO(ENOTSUP);
    if (fsinfo == NULL || file == NULL)
        FYGUE_RET_ERRNO(EIO);

    //TODO: assume that the first cluster will remain the same ?
    if (fygue_fat_open(&(fsinfo->fat), file->path, &desc) != 0)
        FYGUE_RET_ERRNO(ENOENT);
    if (desc.type != FYGUE_FAT_DESC_TYPE_FILE)
        FYGUE_RET_ERRNO(EIO);
    rc = fygue_fat_file_sync(
        &(fsinfo->fat),
        &(desc.file),
        desc.file.cluster_entry
    );
    if (rc != 0)
        FYGUE_RET_ERRNO(EIO);
    return 0;
}
