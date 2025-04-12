#include <string.h>
#include <errno.h>
#include <gint/hardware.h>
#include <gint/defs/util.h>
#include "flash.h"

//---
// Internals
//---

/* _fygue_flash_hw_detect(): detect flash hardware information
 *
 * notes
 *   we cannot easily detect the exact geometry of the flash for the Fugue
 *   FS. For now, simply pseudo-detect hardware information and setup
 *   hardcoded information */
int _fygue_flash_hw_detect(struct fygue_flash *flash)
{
	GAUTOTYPE geometry = &(flash->geometry);

	switch (gint[HWCALC])
	{
		case HWCALC_FXCP400:
			geometry->phy_start = 0x01a20000;
			geometry->fsector_size = FYGUE_FSECTOR_SIZE_128K;
			geometry->fsector_count = 0xf2;
			break;
		case HWCALC_G35PE2:
			geometry->phy_start = 0x003f0000;
			geometry->fsector_size = FYGUE_FSECTOR_SIZE_64K;
			geometry->fsector_count = 0x41;
			break;
		case HWCALC_FXCG50:
			geometry->phy_start = 0x00c80000;
			geometry->fsector_count = 0x9c;
			geometry->fsector_size = FYGUE_FSECTOR_SIZE_128K;
			break;
		case HWCALC_FXCG100:
			geometry->phy_start = 0x00c80000;
			geometry->fsector_size = FYGUE_FSECTOR_SIZE_128K;
			geometry->fsector_count = 0x3d;
			break;
		default:
			errno = ENOTSUP;
			return -1;
	}
	if (geometry->fsector_size == FYGUE_FSECTOR_SIZE_128K) {
		geometry->size = geometry->fsector_count * 0x20000;
		geometry->fcluster_count = geometry->fsector_count * 32;
		geometry->fcluster_per_fsector = 32;
	} else {
		geometry->size = geometry->fsector_count * 0x10000;
		geometry->fcluster_count = geometry->fsector_count * 16;
		geometry->fcluster_per_fsector = 16;
	}
	geometry->phy_end = geometry->phy_start + geometry->size;
	return 0;
}

//---
// Public
//---

/* fygue_flash_initialize(): init flash abstraction */
int fygue_flash_initialize(struct fygue_flash *flash)
{
	if (flash == NULL)
		return -1;
	memset(flash, 0x00, sizeof(struct fygue_flash));
	if (_fygue_flash_hw_detect(flash) != 0)
		return -2;
	if (fygue_flash_cmap_initialize(flash, &(flash->cmap)) != 0)
		return -3;
	return 0;
}

/* fygue_flash_sync(): re-init flash information */
int fygue_flash_sync(struct fygue_flash *flash)
{
	if (flash == NULL)
		return -1;
	if (fygue_flash_cmap_sync(flash, &(flash->cmap)) != 0)
		return -3;
	return 0;
}
