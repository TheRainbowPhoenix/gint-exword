#include <stdlib.h>
#include <string.h>
#include "fygue.h"

//---
// Internals
//---

/* _fygue_dir_fat_read(): read and update internal cache
 *
 * return
 *   -2     no directory found
 *   -1     internal error
 *    0     success */
static int _fygue_dir_fat_read(
    struct fygue_fsinfo *fsinfo,
    struct fygue_descriptor *desc
) {
    struct fygue_fat_dirent fat_dirent;
    struct dirent **dirent;
    void *tmp;
    size_t len;
    int rc;

    rc = fygue_fat_dir_read(
        &(fsinfo->fat),
        &(desc->resolve.dir.fat),
        &fat_dirent
    );
    if (rc <= -3)
        return -2;
    if (rc < 0) {
        errno = EIO;
        return -1;
    }
    tmp = reallocarray(
        desc->resolve.dir.dirent,
        desc->resolve.dir.count + 1,
        sizeof(struct dirent *)
    );
    if (tmp == NULL) {
        errno = ENOMEM;
        return -1;
    }
    desc->resolve.dir.dirent = tmp;
    desc->resolve.dir.count += 1;
    //FIXME: ensure UTF-8/SHIFT-JS
    len = strlen(fat_dirent.name);
    dirent = &(desc->resolve.dir.dirent[desc->resolve.dir.pos]);
    *dirent = calloc(1, sizeof(struct dirent) + len + 2);
    if (*dirent == NULL) {
        desc->resolve.dir.count -= 1;
        errno = ENOMEM;
        return -1;
    }
    //FIXME: ensure UTF-8/SHIFT-JS
    (*dirent)->d_ino  = fat_dirent.cluster_id;
    (*dirent)->d_type = DT_REG;
    memcpy((*dirent)->d_name, fat_dirent.name, len + 1);
    return 0;
}

//---
// Public
//---

/* fygue_dir_read(): directory read implementation
 *
 * notes
 *   - assume that the directory have read permission */
int fygue_dir_read(
    struct fygue_descriptor *desc,
    struct dirent **dirent,
    size_t size
) {
    struct fygue_fsinfo *fsinfo;
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
    if (fygue_descriptor_sync(&fsinfo, desc) != 0) {
        errno = EIO;
        return -1;
    }
    if (desc->resolve.dir.pos >= desc->resolve.dir.count)
    {
        rc = _fygue_dir_fat_read(fsinfo, desc);
        if (rc == -2)
            return 0;
        if (rc < 0)
            return rc;
    }
    *dirent = desc->resolve.dir.dirent[desc->resolve.dir.pos];
    desc->resolve.dir.pos += 1;
    return sizeof *dirent;
}
