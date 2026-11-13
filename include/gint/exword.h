/* Experimental EX-word OS backend (DATAPLUS 5/6).
 * Do not call these services after replacing the OS interrupt/vector state.
 */
#ifndef GINT_EXWORD_H
#define GINT_EXWORD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Exword-native file modes: 1=read, 2=write, 3=read/write.
   File paths use the EX-word OS path format. Returns negative on failure. */
enum { GINT_EXWORD_RD = 1, GINT_EXWORD_WR = 2, GINT_EXWORD_RDWR = 3 };
int gint_exword_open(char const *path, int mode);
int gint_exword_close(int fd);
int gint_exword_read(int fd, void *buffer, int size);
int gint_exword_write(int fd, void const *buffer, int size);
int gint_exword_seek(int fd, int offset, int origin);

#ifdef __cplusplus
}
#endif
#endif
