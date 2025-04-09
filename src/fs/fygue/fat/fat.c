/*
** fygue/fat/initialize    - FAT initialization
*/
#include <string.h>
#include <errno.h>

#include <gint/defs/attributes.h>

#include "fat.h"

//---
// Internals
//---

/* struct _fat_bpb : Bios Parameter Block */
struct _fat_bpb
{
    // common boot structure
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];

    // BIOS parameter Block
    uint16_t BPB_BytsPerSec;
    uint8_t  BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t  BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t  BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;

    // Extended BIOS Parameter Block
    uint8_t  BS_DrvNum;
    uint8_t  BS_Reserved1;
    uint8_t  BS_BootSig;
    uint8_t  BS_VolID[4];
    uint8_t  BS_VolLab[11];
    uint8_t  BS_FilSysType[8];
    uint8_t  _unused[448];

    // Signature
    uint8_t Signature_word[2];
} GPACKED(1);


/* _fygue_fat_sector0() - load the special sector0 information
 *
 * notes
 *   Fugue use the special sector0 to fetch critical flash information
 *   (total logical sector count and number of hidden sector). Then, based
 *   on fecthed information, Fugue select an hardcoded FAT configuration
 *
 * return
 *   -1     unable to load the sector0
 *   -2     unsupported Fugue configuration */
static int _fygue_fat_sector0(struct fygue_fat *fat)
{
    uint8_t *sector;
    int rc;

    if (fygue_fat_sector_get_addr(fat, (void*)&sector, 0) != 0)
        return -1;
    fat->SectorHiddenCount = (
        (sector[0x1c9] << 24) |
        (sector[0x1c8] << 16) |
        (sector[0x1c7] <<  8) |
        (sector[0x1c6] <<  0)
    );
    fat->SectorCount = (
        (sector[0x1cd] << 24) |
        (sector[0x1cc] << 16) |
        (sector[0x1cb] <<  8) |
        (sector[0x1ca] <<  0)
    );
    rc = fygue_fat_sector_get_addr(
        fat,
        &fat->BPB,
        fat->SectorHiddenCount
    );
    if (rc != 0)
        return -2;
    return 0;
}

/* _fygue_fat_configure() - configure the FAT information
 *
 * notes
 *   the objectif is to find the first FAT address and the reserved root
 *   cluster (at the end of FATs since we only have to handle FAT12/16)
 *
 *   note that for FAT12/16 two special cluster entry are added, this is
 *   why we have a manual offset to calculate the number of sector per
 *   FAT */
static int _fygue_fat_configure(struct fygue_fat *fat)
{
    struct _fat_bpb *bpb;
    int RootDirSectors;
    int DataSec;

    bpb = (void *)fat->BPB;

    RootDirSectors  = FAT_WORD(bpb->BPB_RootEntCnt) * 32;
    RootDirSectors += FAT_WORD(bpb->BPB_BytsPerSec) - 1;
    RootDirSectors /= FAT_WORD(bpb->BPB_BytsPerSec);
    DataSec  = FAT_WORD(bpb->BPB_RsvdSecCnt);
    DataSec += FAT_WORD(bpb->BPB_FATSz16) * bpb->BPB_NumFATs;
    DataSec += RootDirSectors;
    DataSec  = FAT_WORD(bpb->BPB_TotSec16) - DataSec;
    fat->ClusterCount = DataSec / bpb->BPB_SecPerClus;
    if (fat->ClusterCount < 4085) {
        fat->Type = FYGUE_FAT_TYPE_FAT12;
    } else if (fat->ClusterCount < 65525) {
        fat->Type = FYGUE_FAT_TYPE_FAT16;
    } else {
        // TODO: (FAT32) maybe gint panic ?
        return -2;
    }

    fat->TotalCapacity  = DataSec * FAT_WORD(bpb->BPB_BytsPerSec);
    fat->RootEntCount   = FAT_WORD(bpb->BPB_RootEntCnt);
    fat->SectorSize     = FAT_WORD(bpb->BPB_BytsPerSec);
    fat->SectorPerClus  = bpb->BPB_SecPerClus;

    fat->FAT0SectorID   = fat->SectorHiddenCount;
    fat->FAT0SectorID  += FAT_WORD(bpb->BPB_RsvdSecCnt);
    fat->FAT1SectorID   = fat->FAT0SectorID;
    fat->FAT1SectorID  += FAT_WORD(bpb->BPB_FATSz16);
    fat->RootSectorID   = fat->FAT1SectorID;
    fat->RootSectorID  += FAT_WORD(bpb->BPB_FATSz16);
    fat->DataSectorID   = fat->RootSectorID;
    fat->DataSectorID  += RootDirSectors;
    return 0;
}

/* _fygue_fat_prepare() - prepare logic information */
static int _fygue_fat_prepare(struct fygue_fat *fat)
{
    int rc = fygue_fat_sector_get_addr(
        fat,
        &fat->RootAddr,
        fat->RootSectorID
    );
    if (rc != 0)
        return -1;
    return 0;
}

//---
// Public
//---

/* fygue_fat_initialize() - fully initialize the FAT information */
int fygue_fat_initialize(struct fygue_fat *fat)
{
    if (fat == NULL)
        return -1;
    memset(fat, 0x00, sizeof(struct fygue_fat));
    if (fygue_flash_initialize(&(fat->_flash)) != 0)
        return -2;
    if (_fygue_fat_sector0(fat) != 0)
        return -3;
    if (_fygue_fat_configure(fat) != 0)
        return -4;
    if (_fygue_fat_prepare(fat) != 0)
        return -5;
    return 0;
}

/* fygue_fat_sync(): re-initialize the FAT information */
int fygue_fat_sync(struct fygue_fat *fat)
{
    if (fat == NULL)
        return -1;
    if (fygue_flash_sync((&fat->_flash)) != 0)
        return -2;
    if (_fygue_fat_prepare(fat) != 0)
        return -3;
    return 0;
}
