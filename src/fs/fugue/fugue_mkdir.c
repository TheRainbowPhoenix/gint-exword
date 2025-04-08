#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/gint.h>
#include <gint/fs.h>
#include <errno.h>
#include "util.h"
#include "bfilecp.h"

int fugue_mkdir(char const *path, GUNUSED mode_t mode)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

#if GINT_OS_CP
	char *normpath = fs_path_normalize_opt(path, "\\fls0\\", '\\');
	if(!normpath) {
		errno = ENOMEM;
		return -1;
	}

    gint_world_enter(GINT_WORLD_OS);
	int rc = BFile_Mkdir(normpath);
    gint_world_leave();
	free(normpath);
#else
	uint16_t *fcpath = fs_path_normalize_fc(path);
	if(!fcpath) {
		errno = ENOMEM;
		return -1;
	}

    gint_world_enter(GINT_WORLD_OS);
	int rc = BFile_Create(fcpath, BFile_Folder, NULL);
    gint_world_leave();
	free(fcpath);
#endif

	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		return -1;
    }
	return rc;
}
