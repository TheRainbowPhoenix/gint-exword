// CP version of the BFile interface, which is much closer to POSIX.
// Functions not available elsewhere that could be useful for optimization:
//  fstat, getAddr

#ifndef GINT_FS_BFILECP_H
#define GINT_FS_BFILECP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>

// Only for CP, obviously.
#if GINT_OS_CP

#define BFileCP_ENOMEM                  -1
#define BFileCP_EINVAL                  -2
#define BFileCP_EDEVFAIL                -3
#define BFileCP_EMOUNTED                -4
#define BFileCP_EACCES                  -5
#define BFileCP_EBADFSID                -6
#define BFileCP_ENOVOLUME               -7
#define BFileCP_ENOPATH                 -8
#define BFileCP_EEXIST                  -9
#define BFileCP_ENAMETOOLONG            -10
#define BFileCP_EOUTOFBOUND             -11
#define BFileCP_EUNFORMAT               -12
#define BFileCP_ENOSPC                  -13
#define BFileCP_ENOENT                  -14
#define BFileCP_EISDIRECTORY            -15
#define BFileCP_ESHARE                  -16
#define BFileCP_EMFILE                  -17
#define BFileCP_EBADF                   -18
#define BFileCP_EEOF                    -19
#define BFileCP_ENOTEMPTY               -20
#define BFileCP_ECLUSTERSIZEMISMATCH    -40
#define BFileCP_ESYSTEM                 -99

#define BFile_ReadOnly      0x01
#define BFile_WriteOnly     0x02
#define BFile_ReadWrite     (BFile_ReadOnly | BFile_WriteOnly)
#define BFile_CreateFlag    0x04
#define BFile_AppendFlag    0x10

int BFile_Open(const char *path, int flags);

int BFile_Read(int fd, void *buf, int size);
int BFile_Write(int fd, void const *buf, int size);

#define BFileCP_SEEK_SET    0
#define BFileCP_SEEK_CUR    1
#define BFileCP_SEEK_END    2
int BFile_Seek(int fd, int offset, int whence);

int BFile_Close(int fd);

int BFile_Flush(int fd);

int BFile_Remove(char const *path);
int BFile_Rename(char const *oldpath, char const *newpath);
int BFile_Mkdir(char const *path);

//---
// Metadata API
//---

// Dates are bitfields { year-1980: 8; month-1-12: 4; day-1-31: 4; }
// Times are bitfields { hour: 5; minute: 6; seconds_div2: 5; }
struct BFile_Statbuf {
    u32 _1;
    u32 size; // file size in bytes
    u16 creationDate;
    u16 creationTime;
    u16 lastModifiedDate;
    u16 lastModifiedTime;
    u16 _2;
    u16 lastAccessDate;
};

int BFile_FStat(int fd, struct BFile_Statbuf *statbuf);

// For API compatibility with older models.
static inline int BFile_Size(int fd) {
    struct BFile_Statbuf statbuf;
    int rc = BFile_FStat(fd, &statbuf);
    return (rc >= 0) ? (int)statbuf.size : rc;
}

int BFile_Stat(const char *path, struct BFile_Statbuf *statbuf);

// Same as legacy, uses find API.
int BFile_Ext_Stat(uint16_t const *path, int *type, int *size);

//---
// Search API
//---

#define BFile_Type_File         1
#define BFile_Type_Directory    5

struct BFile_FileInfo {
    u32 _1;
    u16 type; /* 1=File, 5=Directory */
    u16 _2;
    u32 file_size; /* in bytes; 0 for folders */
    u32 _3, _4;
};

// Same as legacy.
int BFile_FindFirst(u16 const *pattern, int *shandle, u16 *foundfile,
    struct BFile_FileInfo *fileinfo);
int BFile_FindNext(
    int shandle, u16 *foundfile, struct BFile_FileInfo *fileinfo);
int BFile_FindClose(int shandle);

#endif /* GINT_OS_CP */

#ifdef __cplusplus
}
#endif

#endif /* GINT_FS_BFILECP_H */
