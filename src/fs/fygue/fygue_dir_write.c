#include "fygue.h"

/* fygue_dir_write(): write directory (EISDIR) */
ssize_t fygue_dir_write(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir,
	void *buffer,
	size_t size
) {
	if (fsinfo == NULL || dir == NULL)
		FYGUE_RET_ERRNO(EIO);
	if (buffer == NULL)
		FYGUE_RET_ERRNO(EFAULT);
	if (size != 0)
		FYGUE_RET_ERRNO(EISDIR);
	return 0;
}
