#include "flash.h"

/* fygue_flash_sector_get_addr(): return the logical sector address */
int fygue_flash_sector_get_addr(
	struct fygue_flash *flash,
	uintptr_t *sector_addr,
	uint16_t sector_id
) {
	if (flash == NULL || sector_addr == NULL)
		return -1;
	return fygue_flash_cmap_lsector_get_addr(
		flash,
		&(flash->cmap),
		sector_addr,
		sector_id
	);
}
