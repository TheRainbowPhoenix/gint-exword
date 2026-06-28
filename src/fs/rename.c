#include <stdio.h>
#include "fugue/fugue.h"
#include "fygue/fygue.h"

int rename(char const *oldpath, char const *newpath)
{
	/* Standard rename() is the Fugue filesystem only */
	int rc = fugue_rename(oldpath, newpath);
	fygue_set_fs_dirty();
	return rc;
}
