#include <unistd.h>
#include "fugue/fugue.h"

int rmdir(char const *path)
{
	/* Standard rmdir() is the Fugue filesystem only */
	int rc = fugue_rmdir(path);
	fs_fygue_sync();
	return rc;
}
