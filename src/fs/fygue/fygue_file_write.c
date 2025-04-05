#include "fygue.h"

/* fygue_file_write(): write directory (EISDIR) */
ssize_t fygue_file_write(
    struct fygue_descriptor *desc,
    void *buffer,
    size_t size
) {
    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_FILE) {
        errno = EBADF;
        return -1;
    }
    if (buffer == NULL) {
        errno = EFAULT;
        return -1;
    }
    if (size == 0)
        return 0;
    errno = EROFS;
    return -1;
}
