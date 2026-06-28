#include <unistd.h>
#include "fugue/fugue.h"
#include "fygue/fygue.h"

int unlink(char const *path)
{
	/* Standard unlink() is the Fugue filesystem only */
	int rc = fugue_unlink(path);
	fygue_set_fs_dirty();
	return rc;
}
