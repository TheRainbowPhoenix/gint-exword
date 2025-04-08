#include <sys/stat.h>
#include <gint/fs.h>
#include <gint/gint.h>
#include <gint/bfile.h>
#include "fugue.h"
#include <errno.h>
#include <stdlib.h>
#include "util.h"
#include "bfilecp.h"

int fugue_stat(char const * restrict path, struct stat * restrict statbuf)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

	uint16_t *fcpath = fs_path_normalize_fc(path);
	if(!fcpath) {
		errno = ENOMEM;
		return -1;
	}

	int type, size, rc;
    gint_world_enter(GINT_WORLD_OS);
	rc = BFile_Ext_Stat(fcpath, &type, &size);
    gint_world_leave();
	free(fcpath);

	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		return -1;
	}

	statbuf->st_mode = bfile_type_to_mode_t(type) | 0777;
	statbuf->st_size = -1;

	if(S_ISREG(statbuf->st_mode)) {
		statbuf->st_size = size;
	}

	return 0;
}
