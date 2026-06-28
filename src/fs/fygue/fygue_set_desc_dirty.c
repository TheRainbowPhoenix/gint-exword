#include <gint/hardware.h>
#include "fygue.h"

/* fygue_set_desc_dirty() - request file re-synchronisation */
int fygue_set_desc_dirty(void *data)
{
	struct fygue_descriptor *desc;

	if(gint[HWFS] != HWFS_FUGUE)
	   FYGUE_RET_ERRNO(ENOTSUP);
	if (data == NULL)
		FYGUE_RET_ERRNO(EBADF);
	desc = data;
	desc->dirty = true;
	return 0;
}
