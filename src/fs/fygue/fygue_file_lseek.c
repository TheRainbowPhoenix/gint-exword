#include "fygue.h"

/* fygue_file_lseek(): seek file */
off_t fygue_file_lseek(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	off_t offset,
	int whence
) {
	if (fsinfo == NULL || file == NULL)
		FYGUE_RET_ERRNO(EIO);
	if (whence != SEEK_CUR && whence != SEEK_SET && whence != SEEK_END)
		FYGUE_RET_ERRNO(EINVAL);

	if(whence == SEEK_CUR)
		offset += file->cursor;
	if(whence == SEEK_END)
		offset += file->size;
	if(offset < 0 || (unsigned)offset >= file->size + 1)
		FYGUE_RET_ERRNO(EINVAL);

	if (fygue_fat_file_seek(&(fsinfo->fat), &(file->fat), offset) != 0)
		FYGUE_RET_ERRNO(EIO);

	file->cursor = offset;
	return file->cursor;
}
