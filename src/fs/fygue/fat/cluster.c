/*
** fygue/fat/cluster   - FAT cluster handling
*/
#include "fat.h"
#include "flash/flash.h"

//---
// Internals
//---

/* _fygue_fat16_cluster_get_next(): FAT16 cluster handling */
static int _fygue_fat16_cluster_get_next(
	struct fygue_fat *fat,
	int *cluster
) {
	uint16_t *fat0;
	int nb_entry_per_sector;
	int cluster_sector;
	int cluster_real;
	int rc;

	nb_entry_per_sector = fat->SectorSize / 2;
	cluster_sector = *cluster / nb_entry_per_sector;
	cluster_real = *cluster - (cluster_sector * nb_entry_per_sector);
	rc = fygue_fat_sector_get_addr(
		fat,
		(void*)&fat0,
		fat->FAT0SectorID + cluster_sector
	);
	if (rc < 0)
		return -1;
	*cluster = FAT_WORD(fat0[cluster_real]);
	return 0;
}

/* _fygue_fat12_cluster_get_next(): FAT12 cluster handling */
// TODO: better calculus
static int _fygue_fat12_cluster_get_next(
	struct fygue_fat *fat,
	int *cluster
) {
	int fat_index0;
	int fat_index1;
	int fat_offset0;
	int fat_offset1;
	int fat_sector0;
	int fat_sector1;
	uintptr_t fat_data0;
	uintptr_t fat_data1;

	fat_index0  = *cluster + (*cluster / 2);
	fat_index1  = fat_index0 + 1;
	fat_sector0 = fat->FAT0SectorID + (fat_index0 / 512);
	fat_sector1 = fat->FAT0SectorID + (fat_index1 / 512);

	if (fat_sector0 == fat_sector1) {
		if (fygue_fat_sector_get_addr(fat, &fat_data0, fat_sector0) != 0)
			return -1;
		fat_data1 = fat_data0;
		fat_offset0 = fat_index0 - ((fat_index0 / 512) * 512);
		fat_offset1 = fat_offset0 + 1;
	} else {
		if (fygue_fat_sector_get_addr(fat, &fat_data0, fat_sector0) != 0)
			return -1;
		if (fygue_fat_sector_get_addr(fat, &fat_data1, fat_sector1) != 0)
			return -1;
		fat_offset0 = fat_index0 - ((fat_index0 / 512) * 512);
		fat_offset1 = fat_index1 - ((fat_index1 / 512) * 512);
	}

	if (*cluster & 1) {
		*cluster = (
			(((((uint8_t*)fat_data0)[fat_offset0] & 0xf0) >> 4) << 0) |
			(((((uint8_t*)fat_data1)[fat_offset1] & 0xff) >> 0) << 4)
		);
	} else {
		*cluster = (
			(((((uint8_t*)fat_data0)[fat_offset0] & 0xff) >> 0) << 0) |
			(((((uint8_t*)fat_data1)[fat_offset1] & 0x0f) >> 0) << 8)
		);
	}
	if (*cluster >= 0x0ff8)
		*cluster = 0xffff;
	return 0;
}

//---
// Public
//---

/* fygue_fat_cluster_get_next() - find the next cluster ID of a file
 *
 * returns
 *  -7  next cluster unused (0x0000)
 *  -6  next cluster end (0xffff)
 *  -5  internal error
 *  -4  provided cluster unused (0x0000)
 *  -3  provided cluster end (0xffff)
 *  -2  provided invalid cluster (root or "out of fat")
 *  -1  argument error
 *   0  success
 *
 * todo
 *   - ensure FAT1 information is the same ? */
int fygue_fat_cluster_get_next(struct fygue_fat *fat, int *cluster_current)
{
	int cluster;
	int rc;

	if (fat == NULL || cluster_current == NULL)
		return -1;
	cluster = *cluster_current;
	if (cluster == 0xffff)
		return -3;
	if (cluster == 0x0000)
		return -4;
	if (cluster <= 1)
		return -2;
	/* ClusterCount stores the number of data clusters, while valid cluster
	 * identifiers run from 2 to ClusterCount + 1 inclusive. */
	if (cluster > fat->ClusterCount + 1)
		return -2;
	if (fat->Type == FYGUE_FAT_TYPE_FAT12)
		rc = _fygue_fat12_cluster_get_next(fat, &cluster);
	if (fat->Type == FYGUE_FAT_TYPE_FAT16) {
		rc = _fygue_fat16_cluster_get_next(fat, &cluster);
		if (rc == 0 && cluster >= 0xfff8)
			cluster = 0xffff;
	}
	if (rc != 0)
		return -5;
	if (cluster == 0xffff)
		rc = -6;
	if (cluster == 0x0000)
		rc = -7;
	*cluster_current = cluster;
	return rc;
}

/* fygue_fat_cluster_get_sector() - get sector ID from cluster ID */
int fygue_fat_cluster_get_sector(
	struct fygue_fat *fat,
	int cluster_id,
	int *sector_id
) {
	if (fat == NULL || sector_id == NULL)
		return -1;
	if (cluster_id > fat->ClusterCount + 1)
		return -1;
	if (cluster_id <= 1) {
		*sector_id = fat->RootSectorID;
	} else {
		*sector_id = fat->DataSectorID + ((cluster_id - 2) * 8);
	}
	return 0;
}

/* fygue_fat_get_cluster_addr() - get logical sector addr */
int fygue_fat_cluster_get_addr(
	struct fygue_fat *fat,
	uintptr_t *addr,
	int cluster_id
) {
	int sector_id;

	if (fygue_fat_cluster_get_sector(fat, cluster_id, &sector_id) != 0)
		return -1;
	return fygue_fat_sector_get_addr(fat, addr, sector_id);
}
