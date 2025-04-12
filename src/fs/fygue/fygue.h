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
#include <errno.h>
#include "fat/fat.h"

//---
// Public global
//---

/* File descriptor type */
extern const fs_descriptor_type_t fygue_file_descriptor_type;
/* Directory descriptor type */
extern const fs_descriptor_type_t fygue_dir_descriptor_type;

//---
// Public (low-level) API
//---

/* helper to set errno and return -1 */
#define FYGUE_RET_ERRNO(error) \
	do { errno = error; return -1; } while(0);

/* fygue_fsinfo: internal fygue FS information */
struct fygue_fsinfo
{
	struct fygue_fat fat;
	bool dirty;
};

/* fygue_open() - open file or directory */
extern int fygue_open(char const *path, int flags, mode_t mode);


/* fygue_stat() - get file or directory information */
extern int fygue_stat(char const *path, struct stat *statbuf);

/* fygue_sync() - request filesystem re-synchronisation */
extern int fygue_sync(void);

/* fygue_syncfs() - request filesystem and file re-synchronisation */
extern int fygue_syncfs(void *data);

/* fygue_mount() - mount the filesystem */
extern int fygue_mount(struct fygue_fsinfo **fsinfo);

//---
// Internals
//---

/* fygue_file - file information */
struct fygue_file
{
	struct fygue_fat_file fat;
	off_t cursor;
	size_t size;
	char *path;
};

/* fygue_dir - directory information */
struct fygue_dir
{
	struct fygue_fat_dir fat;
	struct dirent **dirent;
	int dirent_idx_max;
	int count;
	int pos;
	char *path;
};

/* fygue_descriptor - fygue file descriptor */
struct fygue_descriptor
{
	enum {
		FYGUE_DESC_TYPE_FILE,
		FYGUE_DESC_TYPE_DIR,
	} type;
	union {
		struct fygue_file file;
		struct fygue_dir dir;
	};
	bool dirty;
};

//---
// Helper
//---

/* fygue_open_resolve() - open file or directory */
extern int fygue_open_resolve(
	char const *path,
	struct fygue_descriptor *desc
);

//---
// Directory interface
//---

/* fygue_dir_open(): directory-specific open */
extern int fygue_dir_open(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir,
	char const * const path,
	struct fygue_fat_dir *fat_dir
);

/* fygue_dir_read(): directory read implementation */
extern ssize_t fygue_dir_read(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir,
	struct dirent **dirent,
	size_t size
);

/* fygue_dir_lseek(): seek directory */
extern off_t fygue_dir_lseek(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir,
	off_t offset,
	int whence
);

/* fygue_dir_write(): write directory (EISDIR) */
extern ssize_t fygue_dir_write(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir,
	void *buffer,
	size_t size
);

/* fygue_dir_close(): close directory */
extern int fygue_dir_close(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir
);

/* fygue_dir_sync(): directory-specific sync */
extern int fygue_dir_sync(
	struct fygue_fsinfo *fsinfo,
	struct fygue_dir *dir
);

//---
// File interface
//---

/* fygue_file_open(): file open */
extern int fygue_file_open(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	char const * const path,
	struct fygue_fat_file *fat_file
);

/* fygue_file_read(): read primitive */
extern ssize_t fygue_file_read(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	void *buffer,
	size_t size
);

/* fygue_file_lseek(): seek directory */
extern off_t fygue_file_lseek(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	off_t offset,
	int whence
);

/* fygue_file_write(): write directory (EROFS) */
extern ssize_t fygue_file_write(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file,
	void *buffer,
	size_t size
);

/* fygue_file_close(): close directory */
extern int fygue_file_close(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file
);

/* fygue_file_sync(): file-specific sync */
extern int fygue_file_sync(
	struct fygue_fsinfo *fsinfo,
	struct fygue_file *file
);

#ifdef __cplusplus
}
#endif

#endif /* FS_FYGUE_H */
