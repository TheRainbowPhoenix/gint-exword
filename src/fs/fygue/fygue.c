#include <stdlib.h>
#include <string.h>
#include <gint/hardware.h>
#include "fygue.h"

//---
// Internals
//---

/* FYGUE_GFDI_CHEK - shorthand for all GFDI prologue */
#define FYGUE_GFDI_CHEK(type)   \
	_fygue_gfdi_ensure_valid(&fsinfo, desc, type)

/* _fygue_gfdi_ensure_valid(): ensure GFDI validity
 *
 * notes:
 *   - assume info is not NULL
 *   - auto mount the FS (if needed) and get information
 *   - ensure file sync if needed */
static int _fygue_gfdi_ensure_valid(
	struct fygue_fsinfo **fsinfo,
	struct fygue_descriptor *desc,
	unsigned int type
) {
	int rc;

	if(gint[HWFS] != HWFS_FUGUE)
	   FYGUE_RET_ERRNO(ENOTSUP);
	if (fsinfo == NULL)
		FYGUE_RET_ERRNO(EIO);
	if (desc == NULL || desc->type != type)
		FYGUE_RET_ERRNO(EBADF);
	if (fygue_mount(fsinfo) != 0)
		FYGUE_RET_ERRNO(EIO);
	if (desc->dirty)
	{
		if (desc->type == FYGUE_DESC_TYPE_DIR)
			rc = fygue_dir_sync(*fsinfo, &(desc->dir));
		if (desc->type == FYGUE_DESC_TYPE_FILE)
			rc = fygue_file_sync(*fsinfo, &(desc->file));
		if (rc != 0)
			FYGUE_RET_ERRNO(EIO);
		desc->dirty = false;
	}
	return 0;
}

//---
// Gint File Descriptor Interface (gfdi) for file
//---

/* fygue_gfdi_file_write(): write file */
static ssize_t fygue_gfdi_file_write(
	struct fygue_descriptor *desc,
	void *buffer,
	size_t size
) {
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_FILE) == 0)
		return fygue_file_write(fsinfo, &(desc->file), buffer, size);
	return -1;
}

/* fygue_gfdi_file_read(): write directory (EISDIR) */
static ssize_t fygue_gfdi_file_read(
	struct fygue_descriptor *desc,
	void *buffer,
	size_t size
) {
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_FILE) == 0)
		return fygue_file_read(fsinfo, &(desc->file), buffer, size);
	return -1;
}

/* fygue_file_lseek(): seek file */
static off_t fygue_gfdi_file_lseek(
	struct fygue_descriptor *desc,
	off_t offset,
	int whence
) {
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_FILE) == 0)
		return fygue_file_lseek(fsinfo, &(desc->file), offset, whence);
	return -1;
}

/* fygue_file_close(): close directory */
static int fygue_gfdi_file_close(struct fygue_descriptor *desc)
{
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_FILE) == 0)
		return fygue_file_close(fsinfo, &(desc->file));
	return -1;
}

//---
// Gint File Descriptor Interface (gfdi) for directory
//---

/* fygue_dir_write(): write directory (EISDIR) */
ssize_t fygue_gfdi_dir_write(
	struct fygue_descriptor *desc,
	void *buffer,
	size_t size
) {
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_DIR) == 0)
		return fygue_dir_write(fsinfo, &(desc->dir), buffer, size);
	return -1;
}

/* fygue_gfdi_dir_read(): readdir */
ssize_t fygue_gfdi_dir_read(
	struct fygue_descriptor *desc,
	struct dirent **dirent,
	size_t size
) {
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_DIR) == 0)
		return fygue_dir_read(fsinfo, &(desc->dir), dirent, size);
	return -1;
}

/* fygue_gfdi_dir_lseek(): seek directory */
off_t fygue_gfdi_dir_lseek(
	struct fygue_descriptor *desc,
	off_t offset,
	int whence
) {
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_DIR) == 0)
		return fygue_dir_lseek(fsinfo, &(desc->dir), offset, whence);
	return -1;
}

/* fygue_gfdi_dir_close(): close directory */
int fygue_gfdi_dir_close(struct fygue_descriptor *desc)
{
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_DIR) == 0)
		return fygue_dir_close(fsinfo, &(desc->dir));
	return -1;
}

//---
// Gint File Descriptor Interface (gfdi) for file or directory
//---

/* fygue_gfdi_syncfs(): flush buffers of the fs */
int fygue_gfdi_syncfs(struct fygue_descriptor *desc)
{
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_DIR) == 0 || FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_FILE) == 0)
		return fygue_syncfs(fsinfo, desc);
	return -1;
}

/* fygue_gfdi_fsync(): flush buffers of the file */
int fygue_gfdi_fsync(struct fygue_descriptor *desc)
{
	struct fygue_fsinfo *fsinfo;

	if (FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_DIR) == 0 || FYGUE_GFDI_CHEK(FYGUE_DESC_TYPE_FILE) == 0)
		return fygue_fsync(fsinfo, desc);
	return -1;
}

//---
// Descriptor
//---

/* fygue_file_descriptor_type - fygue file descriptor */
const fs_descriptor_type_t fygue_file_descriptor_type = {
	.read   = (void*)&fygue_gfdi_file_read,
	.write  = (void*)&fygue_gfdi_file_write,
	.lseek  = (void*)&fygue_gfdi_file_lseek,
	.close  = (void*)&fygue_gfdi_file_close,
	.syncfs = (void*)&fygue_gfdi_syncfs,
	.fsync  = (void*)&fygue_gfdi_fsync,
};

const fs_descriptor_type_t fygue_dir_descriptor_type = {
	.read   = (void*)&fygue_gfdi_dir_read,
	.write  = (void*)&fygue_gfdi_dir_write,
	.lseek  = (void*)&fygue_gfdi_dir_lseek,
	.close  = (void*)&fygue_gfdi_dir_close,
	.syncfs = (void*)&fygue_gfdi_syncfs,
	.fsync  = (void*)&fygue_gfdi_fsync,
};
