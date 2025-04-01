#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/gint.h>
#include <gint/fs.h>
#include <errno.h>
#include "util.h"

int fugue_mkdir(char const *path, GUNUSED mode_t mode)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

	uint16_t *fcpath = fs_path_normalize_fc(path);
	if(!fcpath) {
		errno = ENOMEM;
		return -1;
	}

    gint_world_enter(GINT_WORLD_OS);
	int rc = BFile_Create(fcpath, BFile_Folder, NULL);
	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		free(fcpath);
		rc = -1;
	} else {
        rc = 0;
    }
    gint_world_leave();

	free(fcpath);
	return rc;
}
