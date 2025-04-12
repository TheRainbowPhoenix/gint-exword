#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

off_t lseek(int fd, off_t offset, int whence)
{
	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
		errno = EINVAL;
		return (off_t)-1;
	}

	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return (ssize_t)-1;
	}

	/* No seek function: cannot seek */
	if(d->type->lseek == NULL)
		return 0;
	/* BFile_Seek() can flush pending IO write operations. Sync Fygue's
	 * descriptor to avoid data corruption */
	off_t off = d->type->lseek(d->data, offset, whence);
	if (fs_descriptor_is_fugue(d))
		fs_fygue_sync();
	return off;
}
