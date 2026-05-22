#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/fs.h>
#include <gint/gint.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include "util.h"
#include "fugue.h"
#include "bfilecp.h"

int fugue_rmdir(char const *path)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

	/* Check if the folder is empty */
	DIR *dp = opendir(path);
	if(!dp) return -1;

	bool empty = true;
	struct dirent *ent;

	while((ent = readdir(dp))) {
		if(strcmp(ent->d_name, ".") != 0 &&
		   strcmp(ent->d_name, "..") != 0) {
			empty = false;
			break;
		}
	}

	closedir(dp);

	if(!empty) {
		errno = ENOTEMPTY;
		return -1;
	}

#if GINT_OS_CP
	char *normpath = fs_path_normalize_opt(path, "\\fls0\\", '\\');
#else
	uint16_t *normpath = fs_path_normalize_fc(path);
#endif
	if(!normpath) {
		errno = ENOMEM;
		return -1;
	}

	gint_world_enter(GINT_WORLD_OS);
	int rc = BFile_Remove(normpath);
	gint_world_leave();
	free(normpath);

	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		return -1;
	}
	return 0;
}
