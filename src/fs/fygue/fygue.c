#include <stdlib.h>
#include "fygue.h"

//---
// Internals
//---

/* __fygue_fsinfo: internal filesystem information */
static struct fygue_fsinfo *__fygue_fsinfo = NULL;

//---
// Primitives
//---

/* fygue_mount() - mount (if needed) and return the filesystem information
 *
 * notes
 *   - assume that this global is const after initialisation */
int fygue_mount(struct fygue_fsinfo **fsinfo, bool refresh)
{
    if (fsinfo == NULL)
        return -1;
    if (__fygue_fsinfo == NULL)
    {
        *fsinfo = NULL;
        __fygue_fsinfo = calloc(1, sizeof(struct fygue_fsinfo));
        if (__fygue_fsinfo == NULL)
            return -2;
        if (fygue_fat_init_cold(&(__fygue_fsinfo->fat)) != 0)
            return -3;
        __fygue_fsinfo->dirty = false;
    }
    if (refresh && __fygue_fsinfo->dirty) {
        if (fygue_fat_init_hot(&(__fygue_fsinfo->fat)) != 0)
            return -3;
        __fygue_fsinfo->dirty = false;
    }
    *fsinfo = __fygue_fsinfo;
    return 0;
}

/* fygue_resolve() - try to resolve path
 *
 * notes
 *   - assume that path is clean
 *   - automatically mount the filesystem if needed
 *   - the saved pathname will not be saved in stat */
int fygue_resolve(
    char const * const path,
    int *type,
    struct fygue_stat *stat
) {
    struct fygue_fsinfo *fsinfo;
    struct fygue_fat_dirent dirent;

    if (path == NULL || path[0] != '\\')
        return -1;
    if (fygue_mount(&fsinfo, true) != 0)
        return -2;
    if (fygue_fat_resolve(&(fsinfo->fat), path, &dirent) != 0)
        return -3;
    (void)type;
    (void)stat;
    // todo: convert FAT dirent into Fygue stat
    return 0;
}

//---
// Descriptor
//---

const fs_descriptor_type_t fygue_descriptor_type = {
    .read   = NULL,
    .write  = NULL,
    .lseek  = NULL,
    .close  = NULL,
};

const fs_descriptor_type_t fygue_dir_descriptor_type = {
    .read   = NULL,
    .write  = NULL,
    .lseek  = NULL,
    .close  = NULL,
};
