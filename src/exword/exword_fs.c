/* EX-word OS filesystem bridge.
 * The sys_* function implementations are from libdataplus-ex's syscall/file.c.
 * These native operations are deliberately NOT yet exposed as POSIX open();
 * a descriptor table and flag/errno translation are needed first.
 */
#include <gint/exword.h>
#include <syscalls/syscalls.h>

int gint_exword_open(char const *path, int mode)
{
    if(!path || mode < GINT_EXWORD_RD || mode > GINT_EXWORD_RDWR)
        return -1;
    return sys_open(path, mode);
}
int gint_exword_close(int fd) { return sys_close(fd); }
int gint_exword_read(int fd, void *buf, int size)
{
    if(!buf || size < 0) return -1;
    return sys_read(fd, buf, size);
}
int gint_exword_write(int fd, void const *buf, int size)
{
    if(!buf || size < 0) return -1;
    return sys_write(fd, buf, size);
}
int gint_exword_seek(int fd, int offset, int origin)
{
    return sys_seek(fd, offset, origin);
}
