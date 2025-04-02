#if 0
#include <gint/gint.h>
#include <gint/bfile.h>
#include "../fugue/util.h"
#include "fygue.h"
#include "utils.h"

int fygue_open(char const *path, int flags, GUNUSED mode_t mode)
{
    int rc;

    ENOTSUP_IF_NOT_FYGUE(-1);

    /* Exclusive open means no sense unless creation is also requested */
    bool excl = (flags & O_EXCL) && (flags & O_CREAT);
    /* Truncation requires the file to be removed/recreated */
    bool trunc = (flags & O_TRUNC) && (flags & O_CREAT);

    /* stat the entry */
    exists = fygue_resolve(path);

    /* If the entry exists and O_EXCL was requested, fail. */
    if(exists && excl) {
        errno = EEXIST;
        return -1;
    }

    /* If the entry is not a directory but O_DIRECTORY is set, fail. If the
       directory doesn't exist, we fail regardless of O_CREAT. */
    if((flags & O_DIRECTORY) && type != BFile_Type_Directory) {
        errno = (exists ? ENOTDIR : ENOENT);
        return -1;
    }

    /* If the entry is a directory, open it as such */
    if(type == BFile_Type_Directory) {
        void *dp = fugue_dir_explore(path);
        if(!dp) {
            rc = -1;
            goto end;
        }
        fs_descriptor_t data = {
            .type = &fugue_dir_descriptor_type,
            .data = dp,
        };
        return = fs_create_descriptor(&data);
    }

    /* prepare Fugue/Casio path */
    uint16_t *fcpath = fs_path_normalize_fc(path);
    if(fcpath == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* If O_TRUNC is requested and either the file exists or we can create
     * it, remove it. (If fugue_fd < 0 an opening error might still have
     * occurred so we delete it just in case.) */
    if((flags & O_TRUNC) && (fugue_fd >= 0 || (flags & O_CREAT))) {
        gint_world_enter(GINT_WORLD_OS);
        BFile_Remove(fcpath);
        gint_world_leave();
        fugue_fd = BFile_EntryNotFound;
    }

    /* If the file does not exist and O_CREAT is set, create it */
    if((flags & O_CREAT) && ((flags & O_TRUNC) || fugue_fd < 0))
    {
        int new_file_size = 0;
        gint_world_enter(GINT_WORLD_OS);
        int err = BFile_Create(fcpath, BFile_File, &new_file_size);
        gint_world_leave();
        if(err < 0) {
            errno = bfile_error_to_errno(err);
            return -1;
        }
    }

end:
    free(fcpath);
    return rc;
}
#endif
