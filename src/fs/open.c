#include <fcntl.h>
#include <stdarg.h>
#include "fugue/fugue.h"
#include "fygue/fygue.h"

int open(char const *path, int flags, ...)
{
	va_list args;
	va_start(args, flags);
	mode_t mode = va_arg(args, int);
	va_end(args);

	/* Standard open() use Fugue filesystem if a write operation
     * is requested, otherwise use the Fygue filesystem */
    int rc = 0;
    if ((flags & O_WRONLY) || (flags & O_RDWR)) {
        rc = fugue_open(path, flags, mode);
    } else {
        rc = fygue_open(path, flags, mode);
    }
    /* Even with Fygue open primitive, request a sync for all Fygue's
     * descriptor because when some flags are set (e.g O_CREATE) Fygue
     * invoke BFile_*() syscall that perform IO operation.
     *
     * Note that Fygue's open() primitive with perform a "lazy" open operation
     * by simply ensure that the file exists and mark the file descriptor as
     * dirty to force internal data refresh on the next Fygue's IO
     * operation */
    fs_fygue_sync();
    return rc;
}
