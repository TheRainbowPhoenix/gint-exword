/*
** fygue/fat/cluster   - FAT cluster handling
*/
#include "fat.h"
#include "../flash/flash.h"

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
    uint16_t *fat0;
    int nb_entry_per_sector;
    int cluster_sector;
    int cluster_real;
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
    if (cluster >= fat->ClusterCount)
        return -2;
    nb_entry_per_sector = fat->SectorSize / 2;
    cluster_sector = cluster / nb_entry_per_sector;
    cluster_real = cluster - (cluster_sector * nb_entry_per_sector);
    rc = fygue_fat_sector_get_addr(
        fat,
        (void*)&fat0,
        fat->FAT0SectorID + cluster_sector
    );
    if (rc < 0)
        return -5;
    rc = 0;
    cluster = FAT_WORD(fat0[cluster_real]);
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
    if (cluster_id >= fat->ClusterCount)
        return -1;
    if (cluster_id <= 1) {
        *sector_id = fat->RootSectorID;
    } else {
        *sector_id = fat->DataSectorID + ((cluster_id - 2) * 8);
    }
    return 0;
}
