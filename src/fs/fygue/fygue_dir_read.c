#include <stdlib.h>
#include <string.h>
#include "fygue.h"

/* fygue_dir_read(): directory read implementation
 *
 * notes
 *   - assume that the directory have read permission */
int fygue_dir_read(
    struct fygue_descriptor *desc,
    struct dirent **dirent,
    size_t size
) {
    struct fygue_fat_dirent fat_dirent;
    struct fygue_fsinfo *fsinfo;
    size_t len;
    int rc;

    ENOTSUP_IF_NOT_FYGUE(-1);

    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_DIR) {
        errno = EBADF;
        return -1;
    }
    if (dirent == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (size < sizeof *dirent) {
        errno = EINVAL;
        return -1;
    }
    if (fygue_mount(&fsinfo, true) != 0) {
        errno = EIO;
        return -1;
    }

    /* get the next FAT dirent */
    rc = fygue_fat_readdir(
        &(fsinfo->fat),
        &desc->resolve.dir.fat,
        &fat_dirent
    );
    if (rc <= -3)
        return 0;
    if (rc < 0) {
        errno = EIO;
        return -1;
    }

    /* since `struct dirent` use flexible array to store the filename
     * we need to reallocate the buffer each time */
    //FIXME: ensure UTF-8/SHIFT-JS
    len = strlen(fat_dirent.name);
    *dirent = desc->resolve.dir.dirent;
    *dirent = realloc(*dirent, sizeof(struct dirent) + len + 2);
    if (*dirent == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* convert FAT dirent into generic dirent information */
    //FIXME: ensure UTF-8/SHIFT-JS
    (*dirent)->d_ino  = fat_dirent.cluster_id;
    (*dirent)->d_type = DT_REG;
    memcpy((*dirent)->d_name, fat_dirent.name, len + 1);
    return 0;
}
