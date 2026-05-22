#include <errno.h>
#include "fat.h"

//---
// Internals
//---

/* _get_mode(): get "pseudo" file mode */
static mode_t _get_mode(int attribute)
{
	mode_t mode;

	mode = 0777;
	if (attribute & FYGUE_FAT_ATTR_READ_ONLY)
		mode = 0444;
	return mode | S_IFDIR;
}

/* _get_block_size(): return one cluster size */
static int _get_block_size(struct fygue_fat *fat)
{
	return (fat->SectorSize * fat->SectorPerClus);
}

/* _get_block_nb(): count the number of cluster */
static int _get_block_nb(struct fygue_fat *fat, int cluster)
{
	int count;

	count = 0;
	while (fygue_fat_cluster_get_next(fat, &cluster) == 0)
		count += 1;
	return count;
}

//---
// Public
//---

/* fygue_fat_dir_stat(): directory-specific stat primitive */
int fygue_fat_dir_stat(
	struct fygue_fat *fat,
	struct fygue_fat_dir *dir,
	struct stat *statbuf
) {
	if (fat == NULL || dir == NULL || statbuf == NULL) {
		errno = EIO;
		return -1;
	}
	statbuf->st_dev	 = 0; /* not supported */
	statbuf->st_ino	 = dir->cluster_entry;
	statbuf->st_mode	= _get_mode(dir->attribute);
	statbuf->st_nlink   = 0; /* not supported */
	statbuf->st_uid	 = 0; /* not supported */
	statbuf->st_gid	 = 0; /* not supported */
	statbuf->st_rdev	= 0; /* not supported */
	statbuf->st_size	= 0;
	statbuf->st_atime   = 0; /* not supported */
	statbuf->st_mtime   = 0; /* not supported */
	statbuf->st_ctime   = 0; /* not supported */
	statbuf->st_blksize = _get_block_size(fat);
	statbuf->st_blocks  = _get_block_nb(fat, dir->cluster_entry);
	return 0;
}
