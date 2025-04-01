#include <unistd.h>
#include <gint/fs.h>
#include <errno.h>

int close(int fd)
{
	fs_descriptor_t const *d = fs_get_descriptor(fd);
	if(!d) {
		errno = EBADF;
		return (ssize_t)-1;
	}

	int rc = 0;
	if(d->type->close == NULL)
        goto end;
    /* Fugue's close primitive can flush pending IO write operation. So, we
     * are in the same situation than the write() primitive: force-sync
     * Fygue's descriptor to ensure data intergrity */
	rc = d->type->close(d->data);
    if (fs_descriptor_is_fugue(d))
        fs_fygue_sync();

end:
	fs_free_descriptor(fd);
	return rc;
}
