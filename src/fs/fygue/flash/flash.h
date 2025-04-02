#ifndef FS_FYGUE_FLASH_H
#define FS_FYGUE_FLASH_H 1

#include <gint/defs/types.h>

/* __FYGUE_* - hardcoded fygue information */
#define __FYGUE_FSECTOR_COUNT   (0x9c)
#define __FYGUE_FCLUSTER_COUNT  (__FYGUE_FSECTOR_COUNT * 32)

/* fygue_fcluster - generic flash cluster information */
struct fygue_flash_cluster
{
    uint32_t  kind;
    uint16_t  fcluster_id;
    uint16_t  fcluster_id_bfile;
    uint16_t  lcluster_id;
    uint32_t  version;
    uintptr_t data_p1;
    uintptr_t data_p2;
};

/* fygue_fcluster_map_entry - cmap list entry */
struct fygue_flash_cmap_entry
{
    uint16_t  fcluster_id;
    uint16_t  fcluster_id_bfile;
    uintptr_t fcluster_data_p1;
    uintptr_t fcluster_data_p2;
    uint32_t  version;
};

/* fygue_cmap_info - Cluster redirection information */
struct fygue_flash_cmap
{
    struct fygue_flash_cmap_entry *lcluster;
    int lcluster_id_max;
};

// cluster map interface

/* fygue_flash_cmap_init() - initialize the fcluster redirection map */
extern int fygue_flash_cmap_init(struct fygue_flash_cmap *cmap);

/* fygue_flash_cmap_lsector_get_addr() - get logical sector address */
extern int fygue_flash_cmap_lsector_get_addr(
    struct fygue_flash_cmap *cmap,
    uintptr_t *sector,
    uint16_t lsector_id
);

// cluster interface

/* fygue_flash_cluster_geometry() - get geometry information */
extern int fygue_flash_cluster_geometry(
    uintptr_t addr,
    uint16_t *fcluster_id,
    uint16_t *fsector_id,
    uint16_t *fcluster_off
);

/* fygue_flash_cluster_get() - get cluster information */
extern int fygue_flash_cluster_get(
    uint16_t fcluster_id,
    struct fygue_flash_cluster *fcluster
);

#endif /* FS_FYGUE_FLASH_H */
