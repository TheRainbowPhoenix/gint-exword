#ifndef FS_FYGUE_H
#define FS_FYGUE_H 1

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gint/fs.h>

/* File descriptor type */
extern const fs_descriptor_type_t fygue_descriptor_type;
/* Directory descriptor type */
extern const fs_descriptor_type_t fygue_dir_descriptor_type;

/* Specific implementations of some standard functions */

extern int fygue_open(char const *path, int flags, mode_t mode);

extern int fygue_stat(
    char const * restrict path,
    struct stat * restrict statbuf
);

extern int fygue_syncfs(void *desc);

#endif /* FS_FYGUE_H */
