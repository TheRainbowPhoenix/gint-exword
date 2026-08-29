#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

int fsync(int fd)
{
	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return -1;
	}

	/* BFile commits a write as it takes it, so nothing is held back and no
	   filesystem descriptor needs this today. */
	if(!d->type->fsync)
		return 0;

	return d->type->fsync(d->data);
}
