#include <stdio.h>
#include <gint/fs.h>
#include "fugue/fugue.h"

int rename(char const *oldpath, char const *newpath)
{
	/* Standard rename() is the Fugue filesystem only */
	int rc = fugue_rename(oldpath, newpath);
	fs_fygue_sync();
	return rc;
}
