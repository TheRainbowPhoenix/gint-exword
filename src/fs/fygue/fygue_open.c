#include <string.h>
#include <stdlib.h>
#include <gint/gint.h>
#include <gint/bfile.h>
#include "../fugue/fugue.h"
#include "fygue.h"

/* fygue_open(): open primitive */
int fygue_open(char const *path, int flags, GUNUSED mode_t mode)
{
    struct fygue_descriptor *desc;
    struct fygue_resolve resolve;
    fs_descriptor_t data;

    if ((flags & O_RDWR) || (flags & O_WRONLY)) {
        errno = EROFS;
        return -1;
    }
    ENOTSUP_IF_NOT_FYGUE(-1);

    /* resolve the entry */
    int exists = fygue_resolve(path, &resolve);

    /* if opening fails and no explicit file creation is required, fail */
    if ((exists < 0 && (!(flags & O_CREAT) || (flags & O_DIRECTORY)))) {
        errno = ENOENT;
        return -1;
    }


    /* if opening fails and the previous check as not returned an error,
     * it is certainly because of a creation request. We are a read-only
     * file-system, so, fail */
    if (exists < 0) {
        errno = EROFS;
        return -1;
    }

    /* If the entry exists and O_EXCL was requested, fail.
     * note that Exclusive open means no sense unless creation is also
     * requested */
    if(exists >= 0 && (flags & O_EXCL)) {
        errno = EEXIST;
        return -1;
    }

    /* generate the final file descriptor */
    desc = calloc(1, sizeof(struct fygue_descriptor));
    if (desc == NULL) {
        errno = ENOMEM;
        return -1;
    }
    desc->path = strdup(path);
    desc->flags = flags;
    memcpy(&(desc->resolve), &resolve, sizeof(struct fygue_resolve));
    data.type = &fygue_descriptor_type;
    data.data = desc;
    if (flags & O_DIRECTORY) {
        if (resolve.type != FYGUE_FILE_TYPE_DIR) {
            errno = ENOTDIR;
            return -1;
        }
        data.type = &fygue_dir_descriptor_type;
    }
    return fs_create_descriptor(&data);
}
