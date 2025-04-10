#include <string.h>
#include <stdlib.h>
#include <gint/gint.h>
#include <gint/bfile.h>
#include <gint/hardware.h>
#include "../fugue/fugue.h"
#include "fygue.h"

//---
// Public
//---

/* fygue_open_resolve() - try to resolve path
 *
 * notes
 *   - assume that path is clean
 *   - automatically mount the filesystem if needed
 *   - the saved pathname will not be saved in stat */
int fygue_open_resolve(
    char const * const path,
    struct fygue_descriptor *desc
) {
    struct fygue_fat_descriptor fat_desc;
    struct fygue_fsinfo *fsinfo;

    if (path == NULL || desc == NULL)
        return -1;
    if (fygue_mount(&fsinfo) != 0)
        return -2;
    if (fygue_fat_open(&(fsinfo->fat), path, &fat_desc) != 0)
        return -3;
    memset(desc, 0x00, sizeof(struct fygue_descriptor));
    if (fat_desc.type == FYGUE_FAT_DESC_TYPE_DIR)
    {
        desc->type = FYGUE_DESC_TYPE_DIR;
        return fygue_dir_open(fsinfo, &(desc->dir), path, &(fat_desc.dir));
    }
    desc->type = FYGUE_DESC_TYPE_FILE;
    return fygue_file_open(fsinfo, &(desc->file), path, &fat_desc.file);
}

/* fygue_open(): open primitive */
int fygue_open(char const *path, int flags, GUNUSED mode_t mode)
{
    struct fygue_descriptor desc;
    fs_descriptor_t gint_desc;
    int exists;

    /* preliminary checks */
    if(gint[HWFS] != HWFS_FUGUE)
       FYGUE_RET_ERRNO(ENOTSUP);
    if ((flags & O_RDWR) || (flags & O_WRONLY))
        FYGUE_RET_ERRNO(EROFS);

    /* resolve the entry */
    exists = fygue_open_resolve(path, &desc);

    /* if opening fails and no explicit file creation is required, fail */
    if ((exists < 0 && (!(flags & O_CREAT) || (flags & O_DIRECTORY))))
        FYGUE_RET_ERRNO(ENOENT);

    /* if opening fails and the previous check as not returned an error,
     * it is certainly because of a creation request. We are a read-only
     * file-system, so, fail */
    if (exists < 0)
        FYGUE_RET_ERRNO(EROFS);

    /* If the entry exists and O_EXCL was requested, fail.
     * note that Exclusive open means no sense unless creation is also
     * requested */
    if(exists >= 0 && (flags & O_EXCL))
        FYGUE_RET_ERRNO(EEXIST);

    /* generate the final file descriptor */
    gint_desc.type = &fygue_file_descriptor_type;
    gint_desc.data = calloc(1, sizeof(struct fygue_descriptor));
    if (gint_desc.data == NULL)
        FYGUE_RET_ERRNO(ENOMEM);
    memcpy(gint_desc.data, &desc, sizeof(struct fygue_descriptor));
    if (flags & O_DIRECTORY)
    {
        if (desc.type != FYGUE_DESC_TYPE_DIR)
            FYGUE_RET_ERRNO(ENOTDIR);
        gint_desc.type = &fygue_dir_descriptor_type;
    }
    return fs_create_descriptor(&gint_desc);
}
