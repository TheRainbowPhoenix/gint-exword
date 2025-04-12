#include <errno.h>
#include "fat.h"

/* fygue_fat_file_seek(): seek primitive */
int fygue_fat_file_seek(
	struct fygue_fat *fat,
	struct fygue_fat_file *file,
	off_t offset
) {
	off_t offset_current;

	if (fat == NULL || file == NULL)
		return -1;
	offset_current = 0;
	file->chunk_rd_index = 0;
	file->chunk_rd_offset = 0;
	for (int i = 0; i < file->chunk_count; i++)
	{
		offset_current += file->chunk[i].size;
		if (offset_current > offset) {
			file->chunk_rd_offset = (
				file->chunk[i].size - (offset_current - offset)
			);
			file->cursor = offset;
			return 0;
		}
		file->chunk_rd_index += 1;
	}
	errno = EIO;
	return -1;
}
