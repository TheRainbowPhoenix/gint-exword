#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

ssize_t write(int fd, const void *buf, size_t size)
{
	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return (ssize_t)-1;
	}
	/* No write function: discard the contents but show no error */
	if(d->type->write == NULL)
		return 0;
	/* A special synchronization must be performed to ensure that all
	 * Fygue's descriptor meta information is valid */
	size = d->type->write(d->data, buf, size);
	if (fs_descriptor_is_fugue(d))
		fs_fygue_sync();
	return size;
}
