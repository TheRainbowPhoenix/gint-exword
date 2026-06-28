#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>
#include "fygue/fygue.h"

int syncfs(int fd)
{
    fs_descriptor_t const *d = fs_get_descriptor(fd);
    if(!d) {
        errno = EBADF;
        return -1;
    }
    /* No write function: discard the contents but show no error */
    if(d->type->syncfs == NULL)
        return 0;
    /* A special synchronization must be performed to ensure that all
     * Fygue's descriptor meta information is valid */
    int rc = d->type->syncfs(d->data);
    if (fs_descriptor_is_fugue(d)) {
        struct fygue_fsinfo *fsinfo;

        if (fygue_mount(&fsinfo) != 0)
            return rc;
        fsinfo->dirty = true;
        fygue_mount(&fsinfo);
    }
    return rc;
}
