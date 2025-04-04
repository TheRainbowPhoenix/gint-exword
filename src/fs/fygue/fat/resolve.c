#include <string.h>
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

/* _fugue_dirent_resolve(): convert found dirent to dir */
static int _fygue_dirent_resolve(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir,
    struct fygue_fat_dirent *dirent
) {
    memset(dir, 0x00, sizeof(struct fygue_fat_dir));
    if (dirent == NULL)
    {
        dir->cluster_entry      = 0;
        dir->root_dirent_count  = fat->RootEntCount;
    } else {
        if ((dirent->type & 0x10) == 0x00)
            return -1;
        dir->cluster_entry      = dirent->cluster_id;
        dir->root_dirent_count  = 0;
    }
    dir->dirty           = false;
    dir->sector_count    = 0;
    dir->dirent_cursor   = 0;
    dir->end_of_dirent   = false;
    dir->cluster_current = dir->cluster_entry;
    int rc = fygue_fat_cluster_get_sector(
        fat,
        dir->cluster_current,
        &dir->sector_id
    );
    if (rc != 0)
        return -1;
    return fygue_fat_sector_get_addr(
        fat,
        &dir->dirent_current_addr,
        dir->sector_id
    );
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
    struct fygue_fat_resolve *resolve,
    struct fygue_fat_dirent *dirent
) {
    resolve->type = FYGUE_FAT_FILE_TYPE_DIR;
    if (_fygue_dirent_resolve(fat, &(resolve->dir), dirent) == 0)
        return 0;
    resolve->type = FYGUE_FAT_FILE_TYPE_FILE;
    // todo: support IO file information
    return 0;
}

//---
// Public
//---

/* fygue_fat_resolve(): resolve any path provided
 *
 * notes
 *   - assume that the path is clean */
int fygue_fat_resolve(
    struct fygue_fat *fat,
    char const * const path,
    struct fygue_fat_resolve *resolve
) {
    struct fygue_fat_dirent dirent;
    struct fygue_fat_dir dir;
    char const *prefix;
    char const *start;
    size_t len;
    bool is_root;

    if (fat == NULL || path == NULL || resolve == NULL)
        return -1;
    if (path[0] != '/')
        return -1;
    prefix = path;
    is_root = true;
    while (true)
    {
        if (is_root) {
            if (_fygue_dirent_resolve(fat, &dir, NULL) != 0)
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
        if (_fygue_dirent_resolve(fat, &dir, &dirent) != 0)
            return -5;
    }
    if (is_root)
        return _fygue_resolve_set(fat, resolve, NULL);
    return _fygue_resolve_set(fat, resolve, &dirent);
}
