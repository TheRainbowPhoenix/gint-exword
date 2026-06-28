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

	/* Standard open() uses Fugue filesystem if a write operation
	 * is requested, otherwise uses the Fygue filesystem */
	int rc = -1;
	if ((flags & O_WRONLY) || (flags & O_RDWR)) {
		rc = fugue_open(path, flags, mode);
	} else {
		rc = fygue_open(path, flags, mode);
	}
	return rc;
}
