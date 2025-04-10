#include <errno.h>
#include "fat.h"

//---
// Internals
//---

/* _get_mode(): get "pseudo" file mode */
static mode_t _get_mode(int attribute)
{
    if (attribute & FYGUE_FAT_ATTR_READ_ONLY)
        return S_IFREG | 0444;
    return S_IFREG | 0777;
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

    count = 1;
    while (fygue_fat_cluster_get_next(fat, &cluster) == 0)
        count += 1;
    return count;
}

//---
// Public
//---

/* fygue_fat_file_stat(): directory-specific stat primitive */
int fygue_fat_file_stat(
    struct fygue_fat *fat,
    struct fygue_fat_file *file,
    struct stat *statbuf
) {
    if (fat == NULL || file == NULL || statbuf == NULL) {
        errno = EIO;
        return -1;
    }
    statbuf->st_dev     = 0; /* not supported */
    statbuf->st_ino     = file->cluster_entry;
    statbuf->st_mode    = _get_mode(file->attribute);
    statbuf->st_nlink   = 0; /* not supported */
    statbuf->st_uid     = 0; /* not supported */
    statbuf->st_gid     = 0; /* not supported */
    statbuf->st_rdev    = 0; /* not supported */
    statbuf->st_size    = file->size;
    statbuf->st_atime   = 0; /* not supported */
    statbuf->st_mtime   = 0; /* not supported */
    statbuf->st_ctime   = 0; /* not supported */
    statbuf->st_blksize = _get_block_size(fat);
    statbuf->st_blocks  = _get_block_nb(fat, file->cluster_entry);
    return 0;
}
