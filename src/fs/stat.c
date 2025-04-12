#include <sys/stat.h>
#include "fygue/fygue.h"

int stat(char const * restrict path, struct stat * restrict statbuf)
{
	/* Standard stat() is also provided by Fugue, but rely on Casio's
	 * syscall which do not provide all of the information and request
	 * a world-switch (internally) to works */
	return fygue_stat(path, statbuf);
}
