#include <gint/gint.h>
#include <gint/bfile.h>
#include "bfilecp.h"
#include "fugue.h"

void fugue_flush_all(void)
{
    gint_world_enter(GINT_WORLD_OS);

    for (int i = 0; i < 16; i++) {
        int fd = i;
#if GINT_OS_CP
        fd |= 1 << 24 | 1 << 17;
#endif
        BFile_Flush(fd);
    }

    gint_world_leave();
}

int fugue_syncfs(void *data0)
{
    (void) data0;
    fugue_flush_all();
    return 0;
}
