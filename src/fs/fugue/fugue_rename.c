#include <gint/hardware.h>
#include <gint/bfile.h>
#include <gint/fs.h>
#include <errno.h>
#include <sys/stat.h>
#include "util.h"
#include "bfilecp.h"

int fugue_rename(char const *oldpath, char const *newpath)
{
	ENOTSUP_IF_NOT_FUGUE(-1);

#if GINT_OS_CP
	int rc = BFile_Rename(oldpath, newpath);
#else
	u16 *fcpath_1 = fs_path_normalize_fc(oldpath);
	if(!fcpath_1) {
		errno = ENOMEM;
		return -1;
	}
	u16 *fcpath_2 = fs_path_normalize_fc(newpath);
	if(!fcpath_2) {
		errno = ENOMEM;
		free(fcpath_1);
		return -1;
	}

	int rc = BFile_Rename(fcpath_1, fcpath_2);
	free(fcpath_1);
	free(fcpath_2);
#endif

	if(rc < 0) {
		errno = bfile_error_to_errno(rc);
		return -1;
	}
    return 0;
}
