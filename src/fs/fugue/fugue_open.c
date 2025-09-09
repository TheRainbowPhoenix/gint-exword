#include <gint/fs.h>
#include <gint/bfile.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "util.h"
#include "fugue.h"
#include "bfilecp.h"

GUNUSED static int new_file_size;

int fugue_open(char const *path, int flags, GUNUSED mode_t mode)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

	uint16_t *fcpath = fs_path_normalize_fc(path);
	int fugue_fd, rc=-1, type, fd=-1;

	if(!fcpath) {
		errno = ENOMEM;
		return -1;
	}

#if GINT_OS_CP
	char *normpath = fs_path_normalize_opt(path, "\\fls0\\", '\\');
	if(!normpath) {
		errno = ENOMEM;
		free(fcpath);
		return -1;
	}
# define NORMALIZED_PATH normpath
#else
# define NORMALIZED_PATH fcpath
#endif

	/* Open mode */
	int bfile_mode = BFile_ReadOnly;
	if(flags & O_WRONLY)
		bfile_mode = BFile_WriteOnly;
	else if(flags & O_RDWR)
		bfile_mode = BFile_ReadWrite;
#if GINT_OS_CP
	if(flags & O_APPEND)
		bfile_mode |= BFile_AppendFlag;
#endif

	/* Stat the entry. A special case is needed for the root, which doesn't
	   respond well. fs_path_normalize_fc() normalizes the path so we just
	   have to check for the fixed string "\\fls0\". */
	bool exists;
#if GINT_OS_CP
	if(!memcmp(fcpath, u"\\fls0\\", 14)) {
#else
	if(!memcmp(fcpath, u"\\\\fls0\\", 16)) {
#endif
		exists = true;
		type = BFile_Type_Directory;
	}
	else {
		exists = (BFile_Ext_Stat(fcpath, &type, NULL) == 0);
		if(!exists) type = -1;
	}

	/* O_EXCL: Succeed *only* if we're creating the file. Only has an
	   effect when combined with O_CREAT, unsurprisingly. */
	if(exists && (flags & O_EXCL) && (flags & O_CREAT)) {
		errno = EEXIST;
		rc = -1;
		goto end;
	}

	/* If the entry is not a directory but O_DIRECTORY is set, fail. If the
	   directory doesn't exist, we fail regardless of O_CREAT. */
	if((flags & O_DIRECTORY) && type != BFile_Type_Directory) {
		errno = (exists ? ENOTDIR : ENOENT);
		rc = -1;
		goto end;
	}

	/* If the entry is a directory, open it as such */
	if(type == BFile_Type_Directory) {
		void *dp = fugue_dir_explore(path);
		if(!dp) {
			rc = -1;
			goto end;
		}
		fs_descriptor_t data = {
			.type = &fugue_dir_descriptor_type,
			.data = dp,
		};
		fd = fs_create_descriptor(&data);
		rc = fd;
		goto end;
	}

	/* We now have a regular file. Before opening it, we may need to:
	   1. Delete it to truncate it (BFile never truncates AFAIK)
	      - If O_TRUNC and the file exists (obviously).
	      - Deleting and recreating for truncation purposes is allowed when
	        not setting O_CREAT. In case of error midway, good luck.
	   2. Create it (except fx-CP 400 which can create in BFile_Open())
	      - If it didn't exist and O_CREAT was allowed; or
	      - if it did exist but was removed for truncation in step 1.
	   3. Finally, open the file. */

	/* Delete for truncation. */
	if((flags & O_TRUNC) && exists) {
		BFile_Remove(NORMALIZED_PATH);
		/* Whatever happens now, allow recreating the file. */
		exists = false;
		flags |= O_CREAT;
	}

	/* Create the file now, either directly with BFile_Create(), or with
	   the appropriate flag on fx-CP 400. */
	if((flags & O_CREAT) && !exists) {
#if GINT_OS_CP
		bfile_mode |= BFile_CreateFlag;
#else
		int err = BFile_Create(fcpath, BFile_File, &new_file_size);
		if(err < 0) {
			errno = bfile_error_to_errno(err);
			rc = -1;
			goto end;
		}
#endif
	}

	/* Proceed with the final BFile_Open() call. */
	fugue_fd = BFile_Open(NORMALIZED_PATH, bfile_mode);

	if(fugue_fd < 0) {
		errno = bfile_error_to_errno(fugue_fd);
		rc = -1;
		goto end;
	}

	/* If O_APPEND is set, move to the end of the file */
	// TODO: O_APPEND should move the cursor before *each* write
	int pos = 0;
#if !GINT_OS_CP
	if((flags & O_APPEND)) {
		pos = BFile_Size(fugue_fd);
		BFile_Seek(fugue_fd, pos);
	}
#endif

	/* Return the now-open file descriptor */
	fugue_fd_t *data = malloc(sizeof *data);
	if(!data) {
		BFile_Close(fugue_fd);
		goto end;
	}
	data->fd = fugue_fd;
	data->pos = pos;

	fs_descriptor_t fd_data = {
		.type = &fugue_descriptor_type,
		.data = data,
	};
	rc = fd = fs_create_descriptor(&fd_data);

	if(fd == -1) {
		BFile_Close(fugue_fd);
		free(data);
		errno = ENFILE;
		goto end;
	}

end:
#if GINT_OS_CP
	free(normpath);
#endif
	free(fcpath);
	return rc;
}
