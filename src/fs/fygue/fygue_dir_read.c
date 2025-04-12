#include <stdlib.h>
#include <string.h>
#include "fygue.h"

/* fygue_dir_read(): directory read implementation
 *
 * notes
 *   - assume that the directory have read permission */
ssize_t fygue_dir_read(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir,
	struct dirent **dirent,
	size_t size
) {
	if (fsinfo == NULL || dir == NULL)
		FYGUE_RET_ERRNO(EIO);
	if (dirent == NULL)
		FYGUE_RET_ERRNO(EFAULT);
	if (size < sizeof *dirent)
		FYGUE_RET_ERRNO(EINVAL);
	if (dir->pos >= dir->count)
		FYGUE_RET_ERRNO(EIO);
	*dirent = dir->dirent[dir->pos];
	dir->pos += 1;
	return sizeof *dirent;
}
