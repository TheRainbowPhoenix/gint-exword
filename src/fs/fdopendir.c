#include <gint/fs.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include "fugue/fugue.h"

DIR *fdopendir(int fd)
{
	fs_descriptor_t const *desc = fs_get_descriptor(fd);
	if(desc == NULL) {
		errno = EBADF;
		return NULL;
	}
	if(!fs_descriptor_is_dir(desc)) {
		errno = ENOTDIR;
		return NULL;
	}

	DIR *dp = malloc(sizeof *dp);
	if(!dp) {
		errno = ENOMEM;
		return NULL;
	}

	dp->fd = fd;
	return dp;
}
