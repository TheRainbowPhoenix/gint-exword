#include <sys/stat.h>
#include "fygue/fygue.h"

int stat(char const * restrict path, struct stat * restrict statbuf)
{
	/* Standard stat() is also provided by Fugue, but relies on Casio's
	 * syscall which does not provide all of the information and requests
	 * a world-switch (internally) to work */
	return fygue_stat(path, statbuf);
}
