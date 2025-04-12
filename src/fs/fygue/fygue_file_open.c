#include <string.h>
#include "fygue.h"

/* fygue_file_open(): file open */
int fygue_file_open(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	char const * const path,
	struct fygue_fat_file *fat_file
) {
	if (fsinfo == NULL || file == NULL || path == NULL || fat_file == NULL)
		return -1;

	memset(file, 0x00, sizeof(struct fygue_file));
	file->path = strdup(path);
	if (file->path == NULL)
		FYGUE_RET_ERRNO(ENOMEM);
	file->cursor = 0;
	file->size = fat_file->size;
	memcpy(&(file->fat), fat_file, sizeof(struct fygue_fat_file));
	return 0;
}
