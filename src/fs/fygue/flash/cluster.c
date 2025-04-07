/*
** fygue/_flash/cluster - flash cluster handling
*/
#include <string.h>

#include <gint/defs/attributes.h>

#include "flash.h"

//---
// Internals
//---

/* _bfile_cluster_metadata - special cluster metadata information */
struct _bfile_cluster_metadata
{
    uint8_t     bitmask[2];
    uint32_t    fcluster_version;
    uint16_t    lcluster_id;
    uint16_t    fcluster_crc;
    uint8_t     constant[24];
    uint8_t     signature[3];
    uint8_t     ecc_bitmask_position[3];
} GPACKED(1);

/* _fygue_flash_cluster_convert - ensure that the provided meta is valid
 *
 * notes
 *   - assume that fcluster is not NULL
 *   - try using P1 information first then, P2 if this greedy-read fail
 *   - to ensure that the provided bfile metadata is valid we will perform
 *       some test (also performed by Casio). First, ensure that the
 *       fcluster "kind" is valid (only 0x88, 0x22 or 0x11 are known and
 *       used) and then ensure that the "constant" part is also valid
 *
 *   todo : ensure checksum
 *   todo : ensure ecc handling (?)
 *   todo : try P1 then P2
 *
 * return
 *   -1  if the provided `fcluster` is not NULL
 *   -1  if the bfile metadata cannot be validated
 *    0  otherwise */
static int _fygue_flash_cluster_convert(
    struct fygue_flash *flash,
    struct fygue_flash_cluster *fcluster,
    uintptr_t bfile_meta
) {
    static const uint8_t _constant1[] = {
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff,
    };
    static const uint8_t _constant2[] = {
        0x11, 0x11, 0x11, 0x22,
        0x22, 0x22, 0x44, 0x44,
        0x44, 0x88, 0x88, 0x88,
        0x77, 0x77, 0x77, 0xbb,
        0xbb, 0xbb, 0xdd, 0xdd,
        0xdd, 0xee, 0xee, 0xee,
    };
    struct _bfile_cluster_metadata *meta;

    if (fcluster == NULL)
        return -1;
    meta = (void*)(bfile_meta | 0xa0000000);
    if (
        meta->signature[0] != 0x5a ||
        meta->signature[1] != 0x5a ||
        meta->signature[2] != 0x5a
    ) {
        return -2;
    }
    if (
        meta->bitmask[0] != 0x88 &&
        meta->bitmask[0] != 0x22 &&
        meta->bitmask[0] != 0x11
    ) {
        return -3;
    }
    if (meta->bitmask[1] != 0x0f)
        return -4;
    if (meta->bitmask[0] != 0x11 && meta->lcluster_id != 0xffff)
        return -5;
    // on the fxcp400, have only some flash sector with the constants used
    // in other device. Internally, on the fxcp400, Casio only perform
    // a check an CRC that is performed on the whole cluster
    // TODO: CRC check
    if (
        flash->integrity == FYGUE_FLASH_INTEGRITY_FULL   &&
        memcmp(&(meta->constant), _constant1, 0x18) != 0 &&
        memcmp(&(meta->constant), _constant2, 0x18) != 0
    ) {
        return -6;
    }
    fcluster->lcluster_id = meta->lcluster_id;
    fcluster->kind        = meta->bitmask[0];
    fcluster->version     = meta->fcluster_version;
    return 0;
}

//---
// Public
//---

/* fygue_flash_cluster_geometry() - get geometry information
 *
 * notes
 *   generate flash cluster geometry information. Note that we handle
 *   sector as a full 32 entry table, which differ from logical cluster ID
 *   that ignore the last entry which contain meta information
 *
 * returns
 *   -1     if the provided address is not valid
 *    0     success */
int fygue_flash_cluster_geometry(
    struct fygue_flash *flash,
    uintptr_t addr,
    uint16_t *fcluster_id,
    uint16_t *fsector_id,
    uint16_t *fcluster_off
) {
    if ((addr & 0x0fffffff) < flash->geometry.phy_start)
        return -1;
    addr = (addr & 0x0fffffff) - flash->geometry.phy_start;
    if (fcluster_id != NULL) {
        *fcluster_id  = ((addr & 0xfffe0000) >> 17) * 32;
        *fcluster_id += ((addr & 0x0001ffff) >> 12);
    }
    if (fsector_id != NULL)
        *fsector_id = ((addr & 0xfffe0000) >> 17) * 32;
    if (fcluster_off != NULL)
        *fcluster_off = ((addr & 0x0001ffff) >> 12);
    return 0;
}

/* fygue_flash_cluster_get() - get flash cluster information
 *
 * return
 *   -1  if the provided `fline` is not valid
 *   -2  if the provided `fline` have invalid Bfile metadata
 *    0  if the meta information has been generated */
int fygue_flash_cluster_get(
    struct fygue_flash *flash,
    uint16_t fcluster_id,
    struct fygue_flash_cluster *fcluster
) {
    uintptr_t bfile_meta;
    uintptr_t bfile_data;
    int fsector_id;
    int fcluster_off;

    if (flash == NULL || fcluster == NULL)
        return -1;
    if (fcluster_id >= flash->geometry.fcluster_count)
        return -1;
    fsector_id    = fcluster_id / 32;
    fcluster_off  = fcluster_id % 32;
    if (fcluster_off == 31)
    {
        fcluster->kind          = 0x667;
        fcluster->lcluster_id   = 0xffff;
        fcluster->version       = 0xffffffff;
    } else {
        bfile_meta  = flash->geometry.phy_start;
        bfile_meta += (fsector_id * 0x20000) + 0x1f000;
        bfile_meta += (fcluster_off * 0x40);
        if (_fygue_flash_cluster_convert(flash, fcluster, bfile_meta) != 0)
            return -2;
    }
    bfile_data  = flash->geometry.phy_start;
    bfile_data += (fsector_id * 0x20000);
    bfile_data += (fcluster_off * 0x1000);
    fcluster->data_p1           = bfile_data | 0xa0000000;
    fcluster->data_p2           = bfile_data | 0xa0000000;
    fcluster->fcluster_id_bfile = (fsector_id * 31) + fcluster_off;
    fcluster->fcluster_id       = fcluster_id;
    if (fcluster->kind == 0x22)
    {
        uint32_t *data = (void*)fcluster->data_p1;
        if (
            ((data[0] ^ data[1]) != 0xffffffff) ||
            ((data[2] ^ data[3]) != 0xffffffff) ||
            ((data[4] ^ data[5]) != 0xffffffff)
        ) {
            return -2;
        }
        fcluster->version = data[0];
    }
    return 0;
}
