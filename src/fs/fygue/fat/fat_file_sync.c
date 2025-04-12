#include <stdlib.h>
#include "fat.h"

//---
// Internals
//---

/* _fygue_chunk_table_update(): update chunk table */
static int _fygue_chunk_table_update(
	struct fygue_fat_file *file,
	uintptr_t cluster_addr
) {
	uintptr_t cluster_addr_prev;
	void *tmp;

	if (file->chunk_count > 0)
	{
		cluster_addr_prev  = file->chunk[file->chunk_count - 1].addr;
		cluster_addr_prev += file->chunk[file->chunk_count - 1].size;
		if (cluster_addr == cluster_addr_prev)
		{
			file->chunk[file->chunk_count - 1].size += 8*512;
			file->chunk[file->chunk_count - 1].cluster_count += 1;
			return 0;
		}
	}
	if (file->chunk_count >= file->chunk_table_max_index)
	{
		tmp = reallocarray(
			file->chunk,
			file->chunk_table_max_index + 1,
			sizeof(*file->chunk)
		);
		if (tmp == NULL)
			return -1;
		file->chunk = tmp;
		file->chunk_table_max_index += 1;
	}
	file->chunk[file->chunk_count].size = 8*512;
	file->chunk[file->chunk_count].addr = cluster_addr;
	file->chunk[file->chunk_count].cluster_count = 1;
	file->chunk_count += 1;
	return 0;
}

/* _fygue_chunk_error(): invalidate the chunk table */
static int _fygue_chunk_error(struct fygue_fat_file *file)
{
	if (file->chunk == NULL)
		free(file->chunk);
	file->chunk_table_max_index = 0;
	file->chunk_count = 0;
	return -1;
}

//---
// Public
//---

/* fygue_fat_file_sync(): sync internal information */
int fygue_fat_file_sync(
	struct fygue_fat *fat,
	struct fygue_fat_file *file,
	int cluster_id
) {
	uintptr_t cluster_addr;

	if (fat == NULL || file == NULL)
		return -1;
	file->chunk_count = 0;
	file->cluster_entry = cluster_id;
	while (true)
	{
		if (fygue_fat_cluster_get_addr(fat, &cluster_addr, cluster_id) != 0)
			return _fygue_chunk_error(file);
		if (_fygue_chunk_table_update(file, cluster_addr) != 0)
			return _fygue_chunk_error(file);
		if (fygue_fat_cluster_get_next(fat, &cluster_id) != 0)
			break;
	}
	if (fygue_fat_file_seek(fat, file, file->cursor) != 0)
		return -3;
	return 0;
}
