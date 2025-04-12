#include <string.h>
#include <stdlib.h>
#include "fat.h"

//---
// Internals
//---

/* _fygue_path_get_name(): get the current file name
 *
 * notes:
 *   - assume that prefix is not NULL and clean
 *   - assume that size is not NULL
 *   - assume that prefix can start with '/' */
static int _fygue_path_get_name(
	char const **prefix,
	char const **filename,
	size_t *len
) {
	while ((*prefix)[0] == '/') {
		*prefix = &((*prefix)[1]);
	}
	if ((*prefix)[0] == '\0')
		return -1;
	*len = 0;
	*filename = *prefix;
	while ((*prefix)[0] != '\0' && (*prefix)[0] != '/') {
		*len += 1;
		*prefix = &((*prefix)[1]);
	}
	return 0;
}

/* _fygue_opendir_find() - try to find the directory entry */
static int _fygue_dirent_find(
	struct fygue_fat *fat,
	struct fygue_fat_dir *dir,
	struct fygue_fat_dirent *dirent,
	char const *start,
	size_t len
) {
	while (true)
	{
		if (fygue_fat_dir_read(fat, dir, dirent) != 0)
			return -1;
		if (strncmp(dirent->name, start, len) != 0)
			continue;
		return 0;
	}
}

/* _fygue_resolve_set(): setup the resolve response */
static int _fygue_resolve_set(
	struct fygue_fat *fat,
	struct fygue_fat_descriptor *desc,
	struct fygue_fat_dirent *dirent
) {
	if (dirent == NULL || (dirent->attribute & FYGUE_FAT_ATTR_DIRECTORY)) {
		desc->type = FYGUE_FAT_DESC_TYPE_DIR;
		return fygue_fat_dir_open(fat, &(desc->dir), dirent);
	} else {
		desc->type = FYGUE_FAT_DESC_TYPE_FILE;
		return fygue_fat_file_open(fat, &(desc->file), dirent);
	}
}

//---
// Public
//---

/* fygue_fat_resolve(): resolve any path provided
 *
 * notes
 *   - assume that the path is clean */
int fygue_fat_open(
	struct fygue_fat *fat,
	char const * const path,
	struct fygue_fat_descriptor *desc
) {
	struct fygue_fat_dirent dirent;
	struct fygue_fat_dir dir;
	char const *prefix;
	char const *start;
	size_t len;
	bool is_root;

	if (fat == NULL || path == NULL || desc == NULL)
		return -1;
	prefix = path;
	is_root = true;
	while (true)
	{
		if (is_root) {
			if (fygue_fat_dir_open(fat, &dir, NULL) != 0)
				return -3;
			if (_fygue_path_get_name(&prefix, &start, &len) != 0)
				break;
			is_root = false;
			continue;
		}
		if (_fygue_dirent_find(fat, &dir, &dirent, start, len) != 0)
			return -4;
		if (_fygue_path_get_name(&prefix, &start, &len) != 0)
			break;
		if (fygue_fat_dir_open(fat, &dir, &dirent) != 0)
			return -5;
	}
	if (is_root)
		return _fygue_resolve_set(fat, desc, NULL);
	return _fygue_resolve_set(fat, desc, &dirent);
}
