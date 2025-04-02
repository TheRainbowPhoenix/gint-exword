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
extern int fygue_stat(
    char const * restrict path,
    struct stat * restrict statbuf
);

/* fygue_syncfs() - request filesystem re-synchronisation */
extern int fygue_syncfs(void *desc);

//---
// Internals
//---

#include <gint/hardware.h>
#include <errno.h>

#define ENOTSUP_IF_NOT_FYGUE(rc)        \
   if(                                  \
        (gint[HWFS] != HWFS_FUGUE) ||   \
        (flags & O_WRONLY) ||           \
        (flags & O_RDWR)                \
    ) {                                 \
       errno = ENOTSUP;                 \
       return (rc);                     \
    }

#include "fat/fat.h"

/* fygue_fsinfo: internal fygue FS information */
struct fygue_fsinfo
{
    struct fygue_fat fat;
    bool dirty;
};

/* fygue_stat: internals file information */
struct fygue_stat {
    int type;
    union {
        struct {
            int file;
        };
        struct {
            int dir;
        };
    };
};

/* fygue_file_descriptor: internal file descriptor information */
struct fygue_file_descriptor
{
    bool dirty;
};

/* fygue_mount(): mount and return the filesystem info */
extern int fygue_mount(struct fygue_fsinfo **fsinfo, bool refresh);

#ifdef __cplusplus
}
#endif

#endif /* FS_FYGUE_H */
