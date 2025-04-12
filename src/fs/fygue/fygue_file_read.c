#include "fygue.h"

/* fygue_file_read(): read file data */
ssize_t fygue_file_read(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	void *buffer,
	size_t size
) {
	ssize_t read;

	if (fsinfo == NULL || file == NULL)
		FYGUE_RET_ERRNO(EIO);
	if (buffer == NULL)
		FYGUE_RET_ERRNO(EFAULT);
	if (file->cursor > 0 && (unsigned)file->cursor >= file->size)
		return 0;
	if (file->cursor + size >= file->size)
		size = file->size - file->cursor;
	read = fygue_fat_file_read(&(fsinfo->fat), &(file->fat), buffer, size);
	if (read < 0)
		FYGUE_RET_ERRNO(EIO);
	file->cursor += read;
	return read;
}
