/*
** fygue/_flash/cluster - flash cluster handling
*/
#include <string.h>

#include <gint/defs/attributes.h>
#include <gint/defs/util.h>
#include <gint/hardware.h>

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

uintptr_t debug_meta = 0;

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
    uintptr_t meta
) {
#if 0
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
#endif
    struct _bfile_cluster_metadata *bfile_meta;

    if (flash == NULL || fcluster == NULL)
        return -1;
    bfile_meta = (void*)(meta | 0xa0000000);
    if (
        bfile_meta->signature[0] != 0x5a ||
        bfile_meta->signature[1] != 0x5a ||
        bfile_meta->signature[2] != 0x5a
    ) {
        return -2;
    }
    if (
        bfile_meta->bitmask[0] != 0x88 &&
        bfile_meta->bitmask[0] != 0x22 &&
        bfile_meta->bitmask[0] != 0x11
    ) {
        return -3;
    }
    if (bfile_meta->bitmask[1] != 0x0f)
        return -4;
    if (
        bfile_meta->bitmask[0] != 0x11 &&
        bfile_meta->lcluster_id != 0xffff
    ) {
        return -5;
    }
    // on the fxcp400, have only some flash sector with the constants used
    // in other device. Internally, on the fxcp400, Casio only perform
    // a check an CRC that is performed on the whole cluster
    // TODO: CRC check
#if 0
    if (
        memcmp(&(bfile_meta->constant), _constant1, 0x18) != 0 &&
        memcmp(&(bfile_meta->constant), _constant2, 0x18) != 0
    ) {
        return -6;
    }
#endif
    fcluster->lcluster_id = bfile_meta->lcluster_id;
    fcluster->kind        = bfile_meta->bitmask[0];
    fcluster->version     = bfile_meta->fcluster_version;
    return 0;
}

/* _fygue_flash_cluster_meta(): special metadata cluster handling
 *
 * notes:
 *   metadata cluster are the last flash cluster of a flash sector it does
 *   not have Bfile information. Generate a fake flash cluster with a
 *   special custom kind to allow quick identification */
static int _fygue_flash_cluster_meta(
    struct fygue_flash *flash,
    struct fygue_flash_cluster *fcluster,
    int fsector_id
) {
    GAUTOTYPE geometry = &(flash->geometry);

    memset(fcluster, 0x00, sizeof(struct fygue_flash_cluster));
    fcluster->kind              = FYGUE_FCLUSTER_KIND_METADATA;
    fcluster->lcluster_id       = 0xffff;
    fcluster->fcluster_id       = 0xffff;
    fcluster->fcluster_id_bfile = 0xffff;
    fcluster->version           = 0xffffffff;
    fcluster->meta_addr         = 0xffffffff;
    fcluster->data_addr         = (
        (geometry->phy_start | 0xa0000000) +
        ((fsector_id + 1) * geometry->fsector_size) -
        (8 * 512)
    );
    return 0;
}

/* _fygue_flash_cluster_details(): special flash sector details handling
 *
 * notes:
 *   flash sector details is a particular flash cluster that contains
 *   critical versioning information needed to rebuild Casio's flash
 *   mapping.
 *   However, this particular flash cluster are not represented in the
 *   same way between machines. For example, the G35EII that use flash
 *   sector of 64ko merge the details sector inside the metadata cluster to
 *   keep some space which is not the case with the FXCP400 or CG50 that
 *   use 128ko flash sector and which the first cluster should be a details
 *   one type.
 *
 *   for 64k cluster the 0xf3c0 offset can be calculated as:
 *      0x10000 - (8*512)   = 0xf000  | sector size minus cluster size
 *      15 * 0x40           = 0x3c0   | skip all data cluster meta info */
static int _fygue_flash_cluster_details(
    struct fygue_flash *flash,
    struct fygue_flash_cluster *fcluster,
    int fsector_id
) {
    GAUTOTYPE geometry = &(flash->geometry);
    volatile uint32_t *data32;
    uintptr_t meta;
    uintptr_t data;

    memset(fcluster, 0x00, sizeof(struct fygue_flash_cluster));
    if (geometry->fsector_size == FYGUE_FSECTOR_SIZE_64K) {
        data = (
            (geometry->phy_start | 0xa0000000) +
            (fsector_id * 0x10000) +
            (0xf3c0)
        );
        meta = data + 512;
    } else {
        data = (geometry->phy_start | 0xa0000000) + (fsector_id * 0x20000);
        meta = data + 0x1f000;
    }
    debug_meta = meta;
    if (_fygue_flash_cluster_convert(flash, fcluster, meta) != 0)
        return -1;
    if (fcluster->version != 0xffffffff)
        return -2;
    data32 = (void*)data;
    if (
        ((data32[0] ^ data32[1]) != 0xffffffff) ||
        ((data32[2] ^ data32[3]) != 0xffffffff) ||
        ((data32[4] ^ data32[5]) != 0xffffffff)
    ) {
        return -1;
    }
    fcluster->data_addr = data;
    fcluster->meta_addr = meta;
    fcluster->version = data32[0];
    return 0;
}

/* _fygue_flash_cluster_data(): handle generic data flash cluster
 *
 * notes
 *   flash cluster data (0x11 or 0x88) are handled in the same way between
 *   all device */
static int _fygue_flash_cluster_data(
    struct fygue_flash *flash,
    struct fygue_flash_cluster *fcluster,
    int fcluster_id,
    int fsector_id,
    int fcluster_off
) {
    GAUTOTYPE geometry = &(flash->geometry);
    uintptr_t meta;
    uintptr_t data;

    if (geometry->fsector_size == FYGUE_FSECTOR_SIZE_64K) {
        data = (
            (geometry->phy_start | 0xa0000000) +
            (fsector_id * 0x10000) +
            (fcluster_off * (8 * 512))
        );
        meta = (
            (geometry->phy_start | 0xa0000000) +
            (fsector_id * 0x10000) +
            (0xf000) +
            (fcluster_off * 0x40)
        );
    } else {
        data = (
            (geometry->phy_start | 0xa0000000) +
            (fsector_id * 0x20000) +
            (fcluster_off * (8 * 512))
        );
        meta = (
            (geometry->phy_start | 0xa0000000) +
            (fsector_id * 0x20000) +
            (0x1f000) +
            (fcluster_off * 0x40)
        );
    }
    if (_fygue_flash_cluster_convert(flash, fcluster, meta) != 0)
        return -1;
    fcluster->data_addr         = data;
    fcluster->meta_addr         = meta;
    fcluster->fcluster_id       = fcluster_id;
    fcluster->fcluster_id_bfile = (
        (fsector_id * (geometry->fcluster_per_fsector - 1)) +
        (fcluster_off)
    );
    return 0;
}

//---
// Public
//---

/* fygue_flash_cluster_get() - get flash cluster information
 *
 * return
 *   -1  if the provided `fline` is not valid
 *   -2  if the provided `fline` have invalid Bfile metadata
 *    0  if the meta information has been generated */
int fygue_flash_cluster_get(
    struct fygue_flash *flash,
    struct fygue_flash_cluster *fcluster,
    int fcluster_id
) {
    GAUTOTYPE geometry = &(flash->geometry);
    int fsector_id;
    int fcluster_off;
    int nb_entry;

    if (flash == NULL || fcluster == NULL)
        return -1;
    if (fcluster_id >= flash->geometry.fcluster_count)
        return -1;

    if (geometry->fsector_size == FYGUE_FSECTOR_SIZE_64K) {
        nb_entry     = 16;
        fsector_id   = fcluster_id / 16;
        fcluster_off = fcluster_id % 16;
    } else {
        nb_entry     = 32;
        fsector_id   = fcluster_id / 32;
        fcluster_off = fcluster_id % 32;
    }

    if (fcluster_off == 0)
        return _fygue_flash_cluster_details(flash, fcluster, fsector_id);
    if (fcluster_off == (nb_entry - 1))
        return _fygue_flash_cluster_meta(flash, fcluster, fsector_id);
    return _fygue_flash_cluster_data(
        flash,
        fcluster,
        fcluster_id,
        fsector_id,
        fcluster_off
    );
}

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
    int nb_cluster;

    if ((addr & 0x0fffffff) < flash->geometry.phy_start)
        return -1;
    nb_cluster = flash->geometry.fcluster_per_fsector;
    addr = (addr & 0x0fffffff) - flash->geometry.phy_start;
    if (fcluster_id != NULL) {
        *fcluster_id  = ((addr & 0xfffe0000) >> 17) * nb_cluster;
        *fcluster_id += ((addr & 0x0001ffff) >> 12);
    }
    if (fsector_id != NULL)
        *fsector_id = ((addr & 0xfffe0000) >> 17) * nb_cluster;
    if (fcluster_off != NULL)
        *fcluster_off = ((addr & 0x0001ffff) >> 12);
    return 0;
}
