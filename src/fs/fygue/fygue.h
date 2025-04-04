#ifndef FS_FYGUE_H
#define FS_FYGUE_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gint/fs.h>

//---
// Public global
//---

/* File descriptor type */
extern const fs_descriptor_type_t fygue_descriptor_type;
/* Directory descriptor type */
extern const fs_descriptor_type_t fygue_dir_descriptor_type;

//---
// Public (low-level) API
//---

/* fygue_open() - open file or directory */
extern int fygue_open(char const *path, int flags, mode_t mode);

/* fygue_stat() - get file or directory information */
extern int fygue_stat(char const *path, struct stat *statbuf);

/* fygue_syncfs() - request filesystem re-synchronisation */
extern int fygue_syncfs(void *desc);

//---
// Internals
//---

#include <gint/hardware.h>
#include <errno.h>

#define ENOTSUP_IF_NOT_FYGUE(rc)    \
   if(gint[HWFS] != HWFS_FUGUE) {   \
       errno = ENOTSUP;             \
       return (rc);                 \
    }

#include "fat/fat.h"

/* fygue_fsinfo: internal fygue FS information */
struct fygue_fsinfo
{
    struct fygue_fat fat;
    bool dirty;
};

/* fygue_resolve: internals file information */
struct fygue_resolve {
    enum {
        FYGUE_FILE_TYPE_FILE,
        FYGUE_FILE_TYPE_DIR,
    } type;
    union {
        struct {
            off_t cursor;
            struct fygue_fat_file fat;
        } file;
        struct {
            struct fygue_fat_dir fat;
            struct dirent *dirent;
        } dir;
    };
};

/* fygue_descriptor: internal file descriptor information */
struct fygue_descriptor
{
    struct fygue_resolve resolve;
    char const *path;
    int flags;
    bool dirty;
};

/* fygue_mount(): mount and return the filesystem info */
extern int fygue_mount(struct fygue_fsinfo **fsinfo, bool refresh);


/* fygue_resolve() - try to resolve path */
extern int fygue_resolve(
    char const * const path,
    struct fygue_resolve *resolve
);

/* fygue_dir_read(): directory read implementation */
extern int fygue_dir_read(
    struct fygue_descriptor *desc,
    struct dirent **dirent,
    size_t size
);

#ifdef __cplusplus
}
#endif

#endif /* FS_FYGUE_H */
