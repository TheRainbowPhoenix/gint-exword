#ifndef FS_FYGUE_FAT_H
#define FS_FYGUE_FAT_H 1

#include <gint/defs/types.h>
#include <sys/stat.h>

#include "../flash/flash.h"

/* _FAT_WORD() : helper for 16bit value */
#define FAT_WORD(x) \
    (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8))

/* _FAT_DWORD() : helper for 32bit value */
#define FAT_DWORD(x) (           \
      (((x) & 0xff000000) >> 24)  \
    | (((x) & 0x00ff0000) >> 8)   \
    | (((x) & 0x0000ff00) << 8)   \
    | (((x) & 0x000000ff) << 24)  \
)

/* FYGUE_FAT_ATTR_* : FAT dirent attribute */
#define FYGUE_FAT_ATTR_READ_ONLY 0x01
#define FYGUE_FAT_ATTR_HIDDEN    0x02
#define FYGUE_FAT_ATTR_SYSTEM    0x04
#define FYGUE_FAT_ATTR_VOLUME    0x08
#define FYGUE_FAT_ATTR_DIRECTORY 0x10
#define FYGUE_FAT_ATTR_ARCHIVE   0x20

/* fygue_fat - fat information */
struct fygue_fat
{
    enum {
        FYGUE_FAT_TYPE_FAT12    = 0,
        FYGUE_FAT_TYPE_FAT16    = 1,
    } Type;
    int SectorHiddenCount;
    int SectorCount;
    int SectorSize;
    int SectorPerClus;
    int ClusterCount;
    int TotalCapacity;
    int RootEntCount;
    int FAT0SectorID;
    int FAT1SectorID;
    int RootSectorID;
    int DataSectorID;
    uintptr_t FAT0Addr;
    uintptr_t FAT1Addr;
    uintptr_t RootAddr;
    uintptr_t BPB;
    struct {
        struct fygue_flash_cmap cmap;
    } _flash;
};

/* fygue_fat_dirent - FAT dirent information */
//TODO: flexible name array like the high-level `struct dirent` ?
struct fygue_fat_dirent
{
    char name[256];
    size_t size;
    uintptr_t meta_addr;
    unsigned int cluster_id;
    int attribute;
};

/* fygue_fat_dir - fygue directory information */
struct fygue_fat_dir
{
    int cluster_entry;
    int cluster_current;
    int root_dirent_count;
    int sector_id;
    int sector_count;
    int dirent_cursor;
    uintptr_t dirent_current_addr;
    bool end_of_dirent;
    bool dirty;
    int attribute;
};

/* fygue_fat_file: file information needed to perform IO primitives */
struct fygue_fat_file
{
    int cluster_entry;
    int attribute;
    size_t size;
};

/* fygue_fat_resolve: internal resolve information */
struct fygue_fat_resolve
{
    enum {
        FYGUE_FAT_FILE_TYPE_FILE,
        FYGUE_FAT_FILE_TYPE_DIR,
    } type;
    union {
        struct fygue_fat_file file;
        struct fygue_fat_dir dir;
    };
};

/* fygue_fat_init_cold() - fully initialize the FAT information */
extern int fygue_fat_init_cold(struct fygue_fat *fat);

/* fygue_fat_init_hot() - re-initialize the FAT information */
extern int fygue_fat_init_hot(struct fygue_fat *fat);

/* fygue_fat_resolve() - resolve the path and set the dirent information */
extern int fygue_fat_resolve(
    struct fygue_fat *fat,
    char const * const path,
    struct fygue_fat_resolve *resolve
);

//---
// Directory interface
//---

/* fygue_fat_dir_read(): readdir primitive */
extern int fygue_fat_dir_read(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir,
    struct fygue_fat_dirent *dirent
);

/* fygue_fat_dir_close(): closedir primitive */
extern int fygue_fat_dir_close(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir
);

/* fygue_fat_dir_stat(): directory stat primitive */
extern int fygue_fat_dir_stat(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir,
    struct stat *statbuf
);

//---
// File interface
//---

/* fygue_fat_file_stat(): directory stat primitive */
extern int fygue_fat_file_stat(
    struct fygue_fat *fat,
    struct fygue_fat_file *file,
    struct stat *statbuf
);

//---
// Cluster interface
//---

/* fygue_fat_cluster_get_next() - find the next cluster ID of a file */
extern int fygue_fat_cluster_get_next(
    struct fygue_fat *fat,
    int *cluster_current
);

/* fygue_fat_cluster_get_sector() - get sector ID from cluster ID */
extern int fygue_fat_cluster_get_sector(
    struct fygue_fat *fat,
    int cluster_id,
    int *sector_id
);

/* fygue_fat_get_sector_addr() - get logical sector addr */
extern int fygue_fat_sector_get_addr(
    struct fygue_fat *fat,
    uintptr_t *addr,
    int sector
);

#endif /* FS_FYGUE_FAT_H */
