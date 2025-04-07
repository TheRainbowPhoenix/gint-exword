/*
** fygue/flash/cmap    - flash cluster map
*/
#include <stdlib.h>
#include <string.h>

#include <gint/defs/attributes.h>

#include "flash.h"

//---
// Internals
//---

/* fygue_flash_cmap_update()  - update the cluster redirection map
 *
 * notes
 *   Since Casio use its own methods to handle flash sector and cluster we
 *   need to rebuild the translation page between "logical cluster"
 *   (lcluster) and "flash cluster" (fcluster).
 *
 *   Casio do not erase systematicaly each sector when they become
 *   outdated, instead, they use a `age` mechanism which indicate "when"
 *   the cluster has been created a keep the more recent one
 *
 * return
 *   -1   if the provided information is not valid
 *    0   if the provided lcluster exists and has not been updated
 *    1   if the provided lcluster has been created
 *    2   if the provided lcluster exists and has been updated */
static int fygue_flash_cmap_update(
    struct fygue_flash_cmap *cmap,
    struct fygue_flash_cluster *fcluster
) {
    struct fygue_flash_cmap_entry *entry;

    if (cmap == NULL || fcluster == NULL)
        return -1;
    if (fcluster->kind != 0x11)
        return -1;
    if (cmap->lcluster_id_max > fcluster->lcluster_id)
    {
        if (
            cmap->lcluster[fcluster->lcluster_id].version != 0xffffffff &&
            cmap->lcluster[fcluster->lcluster_id].version
                < fcluster->version
        ) {
            return 1;
        }
    } else {
        cmap->lcluster = realloc(
            cmap->lcluster,
            (fcluster->lcluster_id + 1) * sizeof(
                struct fygue_flash_cmap_entry
            )
        );
        for (
            int i = cmap->lcluster_id_max ;
            i < fcluster->lcluster_id ;
            i++
        ) {
            cmap->lcluster[i].fcluster_id       = 0xffff;
            cmap->lcluster[i].fcluster_id_bfile = 0xffff;
            cmap->lcluster[i].fcluster_data_p1  = 0xffffffff;
            cmap->lcluster[i].fcluster_data_p2  = 0xffffffff;
            cmap->lcluster[i].version           = 0xffffffff;
        }
        cmap->lcluster_id_max = fcluster->lcluster_id + 1;
    }
    entry = &(cmap->lcluster[fcluster->lcluster_id]);
    entry->fcluster_id       = fcluster->fcluster_id;
    entry->fcluster_id_bfile = fcluster->fcluster_id_bfile;
    entry->fcluster_data_p1  = fcluster->data_p1;
    entry->fcluster_data_p2  = fcluster->data_p2;
    entry->version           = fcluster->version;
    return 0;
}

//---
// Public
//---

/* fygue_flash_cmap_init() - initialize fcluster translation
 *
 * notes:
 *   Casio have its own flash handling layer that hook Fugue primitives to
 *   control how/when the hardware will perform IO operation
 *   (read,write,erase). This because the Flash used in recent devices use
 *   sector of 128ko which imply, when you want to update one byte of
 *   memory, you need to:
 *      - copy the whole page in RAM
 *      - perform the modification
 *      - write the new page information
 *
 *   So, todo
 *
 * returns:
 *   -1 if the sector 0 is not found
 *    0 success */
int fygue_flash_cmap_init(
    struct fygue_flash *flash,
    struct fygue_flash_cmap *cmap
) {
    struct fygue_flash_cluster fcluster;
    int fsector;
    int rc;

    if (flash == NULL || cmap == NULL)
        return -1;
    memset(cmap, 0x00, sizeof(struct fygue_flash_cmap));
    for (
        fsector = 0 ;
        fsector < flash->geometry.fsector_count ;
        fsector++
    ) {
        if (fygue_flash_cluster_get(flash, fsector * 32, &fcluster) != 0)
            continue;
        if (fcluster.kind != 0x22)
            continue;
        for (int j = 0 ; j < 31 ; j++)
        {
            rc = fygue_flash_cluster_get(
                flash,
                (fsector * 32) + j,
                &fcluster
            );
            if (rc != 0)
                continue;
            if (fcluster.kind != 0x11)
                continue;
            fygue_flash_cmap_update(cmap, &fcluster);
        }
    }
    if (cmap->lcluster == NULL)
        return -2;
    if (cmap->lcluster[0].fcluster_id == 0xffff)
        return -3;
    return 0;
}

/* fygue_flash_cmap_lsector_get_addr() - get logical sector address */
int fygue_flash_cmap_lsector_get_addr(
    struct fygue_flash *flash,
    struct fygue_flash_cmap *cmap,
    uintptr_t *sector,
    uint16_t lsector_id
) {
    uintptr_t fsector_addr;
    uint16_t lcluster_id;

    if (flash == NULL  || sector == NULL || cmap == NULL)
        return -1;
    if (cmap->lcluster == NULL)
        return -1;
    lcluster_id = lsector_id / 8;
    if (
        cmap->lcluster_id_max < lcluster_id ||
        cmap->lcluster[lcluster_id].fcluster_id == 0xffff
    ) {
        return -2;
    }
    fsector_addr  = cmap->lcluster[lcluster_id].fcluster_data_p1;
    fsector_addr += (lsector_id % 8) * 512;
    *sector = fsector_addr;
    return 0;
}
