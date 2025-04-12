#include "fygue.h"

/* fygue_file_write(): write file (EROFS) */
ssize_t fygue_file_write(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	void *buffer,
	size_t size
) {
	if (fsinfo == NULL || file == NULL)
		FYGUE_RET_ERRNO(EIO);
	if (buffer == NULL)
		FYGUE_RET_ERRNO(EFAULT);
	if (size != 0)
		FYGUE_RET_ERRNO(EROFS);
	return 0;
}
