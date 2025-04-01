#include <unistd.h>
#include "fugue/fugue.h"

int mkdir(char const *path, mode_t mode)
{
	/* Standard mkdir() is the Fugue filesystem only */
	int rc = fugue_mkdir(path, mode);
    fs_fygue_sync();
    return rc;
}
