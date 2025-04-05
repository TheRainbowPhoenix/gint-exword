#include "fygue.h"

//---
// Internals
//---

/* _fygue_dir_discover(): force discover all directory */
static void _fygue_dir_discover(struct fygue_descriptor *desc)
{
    struct dirent *ent = NULL;

    while (true)
    {
        if (fygue_dir_read(desc, &ent, sizeof(ent)) != sizeof(ent))
            break;
    }
}

//---
// Public
//---

/* fygue_dir_lseek(): seek directory */
off_t fygue_dir_lseek(
    struct fygue_descriptor *desc,
    off_t offset,
    int whence
) {
    ENOTSUP_IF_NOT_FYGUE(-1);
    if (desc == NULL || desc->resolve.type != FYGUE_FILE_TYPE_DIR) {
        errno = EBADF;
        return -1;
    }
    if (whence != SEEK_CUR && whence != SEEK_SET && whence != SEEK_END) {
        errno = EINVAL;
        return -1;
    }
    if(whence == SEEK_CUR)
        offset += desc->resolve.dir.pos;
    if(whence == SEEK_END) {
        _fygue_dir_discover(desc);
        offset += desc->resolve.dir.count;
    }
    if(offset < 0 || offset >= desc->resolve.dir.count + 1) {
        errno = EINVAL;
        return -1;
    }
    desc->resolve.dir.pos = offset;
    return desc->resolve.dir.pos;
}
