#include <unistd.h>
#include "fugue/fugue.h"
#include "fygue/fygue.h"

int rmdir(char const *path)
{
	/* Standard rmdir() is the Fugue filesystem only */
	int rc = fugue_rmdir(path);
	fygue_set_fs_dirty();
	return rc;
}
