#ifndef GINT_EXWORD_H
#define GINT_EXWORD_H

#include <stddef.h>
#include <stdint.h>

int exword_sys_sdformat(void);
int exword_sys_totaldiskspace(char const *, unsigned long *);
int exword_sys_freediskspace(char const *, unsigned long *);
int exword_sys_create(char const *, int);
int exword_sys_delete(char const *);
int exword_sys_rename(char const *, char const *);
int exword_sys_open(char const *, int);
int exword_sys_close(int);
int exword_sys_closeall(void);
int exword_sys_get_filesize(int);
int exword_sys_seek(int, int, int);
int exword_sys_read(int, void *, int);
int exword_sys_write(int, void const *, int);
int exword_sys_write2(int, void const *, int, int);
int exword_sys_findfirst(char const *, int *, char *, unsigned long *);
int exword_sys_findnext(int, char *, unsigned long *);
int exword_sys_findclose(int);

void exword_boot_error(char const *stage);

#endif
