/*
** fygue/fat/cluster   - FAT cluster handling
*/
#include "fat.h"

//---
// Public
//---

/* fygue_fat_cluster_next() - find the next cluster ID of a file */
int fygue_fat_cluster_next(struct fygue_fat *fat, int *cluster_current)
{
    uint16_t *fat0;
    int cluster;

    if (fat == NULL || cluster_current == NULL)
        return -1;
    cluster = *cluster_current;
    if (cluster <= 1)
        return -2;
    if (cluster >= fat->ClusterCount)
        return -2;
    fat0 = (void*)fat->FAT0Addr;
    if (fat0[cluster] == 0xffff)
        return -3;
    *cluster_current = fat0[cluster];
    return 0;
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
