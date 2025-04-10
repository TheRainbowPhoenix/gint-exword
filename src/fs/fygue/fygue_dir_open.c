#include <string.h>
#include <stdlib.h>
#include "fygue.h"

//---
// Internals
//---

/* realloc a prepare the internal table */
static int _fygue_dir_open_table_realloc(struct fygue_dir *dir)
{
    void *tmp;

    tmp = reallocarray(
        dir->dirent,
        dir->dirent_idx_max + 16,
        sizeof(struct dirent)
    );
    if (tmp == NULL)
        FYGUE_RET_ERRNO(ENOMEM);
    dir->dirent = tmp;
    dir->dirent_idx_max += 16;
    for (int i = dir->count ; i < dir->dirent_idx_max ; i++)
        dir->dirent[i] = NULL;
    return 0;
}

/* free internal table */
static void _fygue_dir_open_table_free(struct fygue_dir *dir)
{
    if (dir->dirent == NULL)
        return;
    for (int i = 0 ; i < dir->dirent_idx_max ; i++) {
        if (dir->dirent[i] != NULL)
            free(dir->dirent[i]);
    }
    free(dir->dirent);
    dir->dirent = NULL;
}

/* convert FAT dirent to dirent */
static int _fygue_dir_open_convert(
    struct dirent **dirent,
    struct fygue_fat_dirent *fat_dirent
) {
    size_t len;

    len = strlen(fat_dirent->name);
    *dirent = calloc(1, sizeof(struct dirent) + len + 2);
    if (*dirent == NULL)
        FYGUE_RET_ERRNO(ENOMEM);
    (*dirent)->d_ino  = fat_dirent->cluster_id;
    (*dirent)->d_type = DT_REG;
    memcpy((*dirent)->d_name, fat_dirent->name, len + 1);
    return 0;
}

/* _fygue_dir_open_discover(): discover all dirent */
static int _fygue_dir_open_discover(
    struct fygue_fsinfo *fsinfo,
    struct fygue_dir *dir
) {
    struct fygue_fat_dirent dirent;
    int rc;

    while (true)
    {
        rc = fygue_fat_dir_read(&(fsinfo->fat), &(dir->fat), &dirent);
        if (rc > 0)
            return 0;
        if (rc < 0)
            break;
        if (dir->count >= dir->dirent_idx_max) {
            if (_fygue_dir_open_table_realloc(dir) != 0)
                break;
        }
        rc = _fygue_dir_open_convert(&(dir->dirent[dir->count]), &dirent);
        if (rc != 0)
            break;
        dir->count += 1;
    }
    _fygue_dir_open_table_free(dir);
    FYGUE_RET_ERRNO(EIO);
}

//---
// Public
//---


/* fygue_dir_open(): directory-specific open */
int fygue_dir_open(
    struct fygue_fsinfo *fsinfo,
    struct fygue_dir *dir,
    char const * const path,
    struct fygue_fat_dir *fat_dir
) {
    if (fsinfo == NULL || dir == NULL || path == NULL || fat_dir == NULL)
        return -1;

    memset(dir, 0x00, sizeof(struct fygue_dir));
    dir->path = strdup(path);
    if (dir->path == NULL)
        FYGUE_RET_ERRNO(ENOMEM);
    dir->dirent_idx_max = 0;
    dir->dirent = NULL;
    dir->count = 0;
    dir->pos = 0;
    memcpy(&(dir->fat), fat_dir, sizeof(struct fygue_fat_dir));

    if (_fygue_dir_open_discover(fsinfo, dir) != 0) {
        free(dir->path);
        return -1;
    }
    return 0;
}
