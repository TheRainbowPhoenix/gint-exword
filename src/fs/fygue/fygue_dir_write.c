#include "fygue.h"

/* fygue_dir_write(): write directory (EISDIR) */
ssize_t fygue_dir_write(
    struct fygue_descriptor *desc,
    void *buffer,
    size_t size
) {
    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_DIR) {
        errno = EBADF;
        return -1;
    }
    if (buffer == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (size == 0)
        return 0;
    errno = EISDIR;
    return -1;
}
