#ifndef FS_FYGUE_FLASH_H
#define FS_FYGUE_FLASH_H 1

#include <gint/defs/types.h>
#include <gint/config.h>

/* fygue_fcluster - generic flash cluster information */
struct fygue_flash_cluster
{
    enum {
        FYGUE_FCLUSTER_KIND_DATA     = 0x11,
        FYGUE_FCLUSTER_KIND_FINFO    = 0x22,
        FYGUE_FCLUSTER_KIND_DETAILS  = 0x88,
        FYGUE_FCLUSTER_KIND_METADATA = 0x667,
    } kind;
    uint16_t  fcluster_id;
    uint16_t  fcluster_id_bfile;
    uint16_t  lcluster_id;
    uint32_t  version;
    uintptr_t data_addr;
    uintptr_t meta_addr;
};

/* fygue_fcluster_map_entry - cmap list entry */
struct fygue_flash_cmap_entry
{
    uint16_t  fcluster_id;
    uint16_t  fcluster_id_bfile;
    uintptr_t fcluster_addr;
    uint32_t  version;
};

/* fygue_cmap_info - Cluster redirection information */
struct fygue_flash_cmap
{
    struct fygue_flash_cmap_entry *lcluster;
    int lcluster_id_max;
};

/* fygue_flash - flash information */
struct fygue_flash
{
    struct {
        uintptr_t phy_start;
        uintptr_t phy_end;
        size_t size;
        enum {
            FYGUE_FSECTOR_SIZE_64K,
            FYGUE_FSECTOR_SIZE_128K,
        } fsector_size;
        int fsector_count;
        int fcluster_per_fsector;
        int fcluster_count;
    } geometry;
    struct fygue_flash_cmap cmap;
};

//---
// (public) Flash interface
//---

/* fygue_flash_initialize(): init flash abstraction */
extern int fygue_flash_initialize(struct fygue_flash *flash);

/* fygue_flash_sync(): re-init flash information */
extern int fygue_flash_sync(struct fygue_flash *flash);

/* fygue_flash_sector_get_addr(): return the logical sector address */
extern int fygue_flash_sector_get_addr(
    struct fygue_flash *flash,
    uintptr_t *sector_addr,
    uint16_t sector_id
);

//---
// (internal) cluster map interface
//---

/* fygue_flash_cmap_initialize() - initialize the fcluster redirection map */
extern int fygue_flash_cmap_initialize(
    struct fygue_flash *flash,
    struct fygue_flash_cmap *cmap
);

/* fygue_flash_cmap_sync() - sync the fcluster redirection map */
extern int fygue_flash_cmap_sync(
    struct fygue_flash *flash,
    struct fygue_flash_cmap *cmap
);

/* fygue_flash_cmap_lsector_get_addr() - get logical sector address */
extern int fygue_flash_cmap_lsector_get_addr(
    struct fygue_flash *flash,
    struct fygue_flash_cmap *cmap,
    uintptr_t *sector,
    uint16_t lsector_id
);

//---
// (internal) cluster interface
//---

/* fygue_flash_cluster_geometry() - get geometry information */
extern int fygue_flash_cluster_geometry(
    struct fygue_flash *flash,
    uintptr_t addr,
    uint16_t *fcluster_id,
    uint16_t *fsector_id,
    uint16_t *fcluster_off
);

/* fygue_flash_cluster_get() - get cluster information */
extern int fygue_flash_cluster_get(
    struct fygue_flash *flash,
    struct fygue_flash_cluster *fcluster,
    int fcluster_id
);

#endif /* FS_FYGUE_FLASH_H */
