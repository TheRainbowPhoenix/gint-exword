/*
** fygue/_fat/sector	- FAT sector handling
*/
#include "fat.h"

//---
// Public
//---

/* fygue_fat_sector_get_addr() - get logical sector addr */
int fygue_fat_sector_get_addr(
	struct fygue_fat *fat,
	uintptr_t *addr,
	int sector
) {
	if (fat != NULL)
		return fygue_flash_sector_get_addr(&(fat->_flash), addr, sector);
	return -1;
}
