#include <stdlib.h>
#include "fygue.h"
#include "../fugue/fugue.h"

//---
// Internals
//---

/* __fygue_fsinfo: internal filesystem information */
static struct fygue_fsinfo *__fygue_fsinfo = NULL;

//---
// Public
//---

/* fygue_mount() - mount (if needed) and return the filesystem information
 *
 * notes
 *   - assume that `__fygue_fsinfo` is const after initialisation */
int fygue_mount(struct fygue_fsinfo **fsinfo)
{
	if (fsinfo == NULL)
		return -1;
	*fsinfo = NULL;
	if (__fygue_fsinfo == NULL)
	{
		__fygue_fsinfo = calloc(1, sizeof(struct fygue_fsinfo));
		if (__fygue_fsinfo == NULL)
			return -2;
		fugue_flush_all();
		if (fygue_fat_initialize(&(__fygue_fsinfo->fat)) != 0)
		{
			free(__fygue_fsinfo);
			__fygue_fsinfo = NULL;
			return -3;
		}
		__fygue_fsinfo->dirty = false;
	}
	if (__fygue_fsinfo->dirty)
	{
		fugue_flush_all();
		if (fygue_fat_sync(&(__fygue_fsinfo->fat)) != 0)
			return -4;
		for (int i = 0 ; i < FS_FD_MAX ; i++) {
			fs_descriptor_t const* desc = fs_get_descriptor(i);
			if (fs_descriptor_is_fygue(desc))
				fygue_set_desc_dirty(desc->data);
		}
		__fygue_fsinfo->dirty = false;
	}
	*fsinfo = __fygue_fsinfo;
	return 0;
}
