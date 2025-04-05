/*
** fygue/fat/readdir   - readdir primitive
*/
#include <alloca.h>

#include <gint/defs/attributes.h>

#include "fat.h"

//---
// Internals
//---

// types

/* _fat_dirent - FAT directory entry structure */
struct _fat_dirent
{
    uint8_t  DIR_Name[11];
    uint8_t  DIR_Attr;
    uint8_t  DIR_NTRes;
    uint8_t  DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} GPACKED(1);

/* _fat_dir_name : Fugue directory name strcuture */
struct _fat_dirname
{
    struct {
        uint8_t         :1; // must be 0
        uint8_t last    :1;
        uint8_t         :1; // must be 0
        uint8_t id      :5;
    } DIR_Sequence;
    uint16_t DIR_Char0;
    uint16_t DIR_Char1;
    uint16_t DIR_Char2;
    uint16_t DIR_Char3;
    uint16_t DIR_Char4;
    uint8_t  DIR_Attr;      // must be 0x0f
    uint8_t  DIR_Type;      // must be 0x00
    uint8_t  checksum;
    uint16_t DIR_Char5;
    uint16_t DIR_Char6;
    uint16_t DIR_Char7;
    uint16_t DIR_Char8;
    uint16_t DIR_Char9;
    uint16_t DIR_Char10;
    uint16_t DIR_FstClus;   // must be 0x0000
    uint16_t DIR_Char11;
    uint16_t DIR_Char12;
} GPACKED(1);

/* _fat_info - internal struct info */
struct _fat_info
{
    struct fygue_fat *fat;
    struct fygue_fat_dir *dir;
    struct fygue_fat_dirent *current;
    uint8_t *filename;
    int idx;
};


/* _fygue_fat_readdir_cursor_update() - update internal directory cursor
 *
 * notes
 *   - assume that dir is always non-null
 *   - special root handling (root= dir->cluster_id == 0)
 *
 * return
 *   -1     end of dirent
 *    0     success */
static int _fygue_fat_readdir_cursor_update(struct _fat_info *finfo)
{
    struct fygue_fat_dir *dir;
    int rc;

    dir = finfo->dir;
    if (dir->end_of_dirent == true)
        return -1;
    dir->end_of_dirent = true;
    if (dir->cluster_entry == 0)
    {
        dir->dirent_cursor += 1;
        if (dir->dirent_cursor >= dir->root_dirent_count)
            return -1;
        if ((dir->dirent_cursor % 16) != 0)
        {
            dir->dirent_current_addr += sizeof(struct _fat_dirent);
            dir->end_of_dirent = false;
            return 0;
        }
        dir->sector_id += 1;
    } else {
        dir->dirent_cursor += 1;
        if ((dir->dirent_cursor % 16) != 0)
        {
            dir->dirent_current_addr += sizeof(struct _fat_dirent);
            dir->end_of_dirent = false;
            return 0;
        }
        dir->dirent_cursor = 0;
        dir->sector_count += 1;
        if (dir->sector_count >= 8)
        {
            rc = fygue_fat_cluster_get_next(
                finfo->fat,
                &dir->cluster_current
            );
            if (rc != 0)
                return -1;
            rc = fygue_fat_cluster_get_sector(
                finfo->fat,
                dir->cluster_current,
                &dir->sector_id
            );
            if (rc != 0)
                return -1;
            dir->sector_count = 0;
        }
    }
    rc = fygue_fat_sector_get_addr(
        finfo->fat,
        &dir->dirent_current_addr,
        dir->sector_id
    );
    if (rc != 0)
        return -1;
    dir->end_of_dirent = false;
    return 0;
}

//---
// FAT/VFAT dirent handling
//---

/* _fygue_fat_readdir_dirent_gen() - generate dirent */
static void _fygue_fat_readdir_dirent_gen(
    struct _fat_info *finfo,
    struct _fat_dirent *dirent
) {
    struct fygue_fat_dirent *current;
    int idx;

    idx = 0;
    current = finfo->current;
    while (finfo->idx > 0)
        current->name[idx++] = finfo->filename[--finfo->idx];
    current->name[idx] = 0x00;
    current->attribute = dirent->DIR_Attr;
    current->size = FAT_DWORD(dirent->DIR_FileSize);
    current->meta_addr = (uintptr_t)dirent;
    current->cluster_id = (
        (FAT_WORD(dirent->DIR_FstClusHI) << 16) |
        (FAT_WORD(dirent->DIR_FstClusLO) << 0)
    );
}

/* _fygue_fat_readdir_name_shard_update() - update internal name info */
static int _fygue_fat_readdir_name_shard_update(
    struct _fat_info *finfo,
    uint16_t buffer[13]
) {
    uint8_t b0;
    uint8_t b1;

    for (int i = 0 ; i < 13 ; i++)
    {
        if (buffer[i] == 0x0000 || buffer[i] == 0xffff)
            continue;
        if (buffer[i] >= 0x20 && buffer[i] <= 0x7a) {
            finfo->filename[finfo->idx++] = buffer[i] & 0x00ff;
            continue;
        }
        b0 = (buffer[i] & 0x00ff) >> 0;
        b1 = (buffer[i] & 0xff00) >> 8;
        if (
            (b0 >= 0x81 && b0 <= 0x9f) ||
            (b0 >= 0xa1 && b0 <= 0xfc)
        ) {
            if (b1 >= 0x40 && b1 <= 0xfc && b1 != 0x7f)
            {
                finfo->filename[finfo->idx++] = b1;
                finfo->filename[finfo->idx++] = b0;
                continue;
            }
        }
        return -2;
    }
    return 0;
}

/* _fygue_fat_readdir_name_prepare() - classic FAT handling */
static int _fygue_fat_readdir_name_fat_prepare(
    struct _fat_info *finfo,
    struct _fat_dirent *dirent
) {
    uint16_t buffer[13];
    bool need_dot;
    int j;

    j = 13;
    need_dot = false;
    for (int i = 0 ; i < 11 ; i++)
    {
        if (i == 8)
            need_dot = true;
        if (dirent->DIR_Name[i] == 0x20)
            continue;
        if (need_dot) {
            buffer[--j] = '.';
            need_dot = false;
        }
        if (dirent->DIR_Name[i] >= 'A' && dirent->DIR_Name[i] <= 'Z') {
            buffer[--j] = dirent->DIR_Name[i] + 0x20;
        } else {
            buffer[--j] = dirent->DIR_Name[i];
        }
    }
    while (--j >= 0)
        buffer[j] = 0x0000;
    finfo->idx = 0;
    return _fygue_fat_readdir_name_shard_update(finfo, buffer);
}

/* _fygue_fat_readdir_name_update() - pre-generate filename info */
static int _fygue_fat_readdir_name_vfat_prepare(
    struct _fat_info *finfo,
    struct _fat_dirname *dirname
) {
    uint16_t buffer[13];

    buffer[0]  = FAT_WORD(dirname->DIR_Char12);
    buffer[1]  = FAT_WORD(dirname->DIR_Char11);
    buffer[2]  = FAT_WORD(dirname->DIR_Char10);
    buffer[3]  = FAT_WORD(dirname->DIR_Char9);
    buffer[4]  = FAT_WORD(dirname->DIR_Char8);
    buffer[5]  = FAT_WORD(dirname->DIR_Char7);
    buffer[6]  = FAT_WORD(dirname->DIR_Char6);
    buffer[7]  = FAT_WORD(dirname->DIR_Char5);
    buffer[8]  = FAT_WORD(dirname->DIR_Char4);
    buffer[9]  = FAT_WORD(dirname->DIR_Char3);
    buffer[10] = FAT_WORD(dirname->DIR_Char2);
    buffer[11] = FAT_WORD(dirname->DIR_Char1);
    buffer[12] = FAT_WORD(dirname->DIR_Char0);
    return _fygue_fat_readdir_name_shard_update(finfo, buffer);
}

//---
// dirent handling
//---

/* _fygue_readdir_vfat() - handle VFAT dirent generation
 *
 * todo
 *   - properly handle broken chain (0x00, 0xe5, ...)
 *   - properly handle `cursor_update()` error
 * returns
 *   -3     internal error
 *   -2     invalid VFAT
 *   -1     invlaid VFAT (start)
 *    0     success */
static int _fygue_readdir_vfat(struct _fat_info *finfo)
{
    struct _fat_dirname *dirname;
    int count;

    dirname = (void*)finfo->dir->dirent_current_addr;
    if (dirname->DIR_Sequence.last == 0) {
        finfo->dir->dirty = true;
        return -1;
    }
    finfo->idx = 0;
    count = dirname->DIR_Sequence.id;
    do {
        dirname = (void*)finfo->dir->dirent_current_addr;
        if (dirname->DIR_Attr != 0x0f)
            return -2;
        if (dirname->DIR_Sequence.id != count)
            return -2;
        if (_fygue_fat_readdir_name_vfat_prepare(finfo, dirname) != 0)
            return -2;
        if (_fygue_fat_readdir_cursor_update(finfo) != 0)
            return -3;
        count -= 1;
    } while (count >= 1);
    _fygue_fat_readdir_dirent_gen(
        finfo,
        (void*)finfo->dir->dirent_current_addr
    );
    finfo->dir->dirty = true;
    return 0;
}

/* _fygue_readdir_fat() - handle FAT dirent generation */
static int _fygue_readdir_fat(struct _fat_info *finfo)
{
    struct _fat_dirent *dirent;

    dirent = (void*)finfo->dir->dirent_current_addr;
    _fygue_fat_readdir_name_fat_prepare(finfo, dirent);
    _fygue_fat_readdir_dirent_gen(finfo, dirent);
    finfo->dir->dirty = true;
    return 0;
}

/* _fygue_readdir_fat_check() - check FAT type
 *
 * notes
 *   check special first character of the FAT entry and try to determine
 *   if it is:
 *     0x00   unused entry and all remaning entry will be unused
 *     0xe5   unused entry
 *     0x2e   special "." or ".." entry
 *  then check the dirent attribute to distinguish an VFAT entry from an
 *  classic FAT entry
 *
 * return
 *   -4     unsupported enty (volume ID)
 *   -3     this is a end-of-dirent entry (0x00)
 *   -2     this is an unused entry (0xe5)
 *   -1     this is a special "." or ".." entry
 *    0     this is a classic FAT entry
 *    1     this is a VFAT entry */
static int _fygue_readdir_fat_check(struct _fat_info *finfo)
{
    struct _fat_dirent *dirent;

    dirent = (void*)finfo->dir->dirent_current_addr;
    switch (dirent->DIR_Name[0])
    {
        case 0x00:
            finfo->dir->end_of_dirent = true;
            return -3;
        case 0xe5:
            finfo->dir->dirty = true;
            return -2;
        case 0x2e:
            if (dirent->DIR_Name[1] == '.') {
                finfo->filename[0] = '.';
                finfo->filename[1] = '.';
                finfo->filename[2] = '\0';
                finfo->idx = 2;
            } else {
                finfo->filename[0] = '.';
                finfo->filename[1] = '\0';
                finfo->idx = 1;
            }
            _fygue_fat_readdir_dirent_gen(finfo, dirent);
            finfo->dir->dirty = true;
            return -1;
    }
    if (dirent->DIR_Attr == 0x0f)
        return 1;
    if ((dirent->DIR_Attr & 0x08) != 0) {
        finfo->dir->dirty = true;
        return -4;
    }
    return 0;
}

//---
// Public
//---

/* fygue_fat_dir_read() - readdir-like primitive
 *
 * return
 *   -3     no dirent remaning
 *   -2     internal error
 *   -1     argument error
 *    0     success */
int fygue_fat_dir_read(
    struct fygue_fat *fat,
    struct fygue_fat_dir *dir,
    struct fygue_fat_dirent *current
) {
    struct _fat_info finfo;
    int rc;

    if (fat == NULL || current == NULL || dir == NULL)
        return -1;
    finfo.fat       = fat;
    finfo.current   = current;
    finfo.dir       = dir;
    finfo.filename  = alloca(256 * sizeof(uint8_t));
    if (finfo.filename == NULL)
        return -2;
    while (dir->end_of_dirent == false)
    {
        if (finfo.dir->dirty)
        {
            if (_fygue_fat_readdir_cursor_update(&finfo) != 0)
                break;
            finfo.dir->dirty = false;
        }
        rc = _fygue_readdir_fat_check(&finfo);
        if (rc == -3)
            break;
        if (rc == -1)
            return 0;
        if (rc < 0)
            continue;
        if (rc == 0 && _fygue_readdir_fat(&finfo) == 0)
            return 0;
        if (rc == 1 && _fygue_readdir_vfat(&finfo) == 0)
            return 0;
    }
    dir->end_of_dirent = true;
    return -3;
}
