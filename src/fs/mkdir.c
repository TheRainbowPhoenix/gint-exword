#include <unistd.h>
#include "fugue/fugue.h"
#include "fygue/fygue.h"

int mkdir(char const *path, mode_t mode)
{
	/* Standard mkdir() is the Fugue filesystem only */
	int rc = fugue_mkdir(path, mode);
	fygue_set_fs_dirty();
	return rc;
}
