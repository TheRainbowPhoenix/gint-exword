#include <unistd.h>
#include <gint/fs.h>
#include "fugue/fugue.h"

int unlink(char const *path)
{
	/* Standard unlink() is the Fugue filesystem only */
	int rc = fugue_unlink(path);
    fs_fygue_sync();
    return rc;
}
