#include <string.h>
#include <errno.h>
#include <gint/hardware.h>
#include "flash.h"

//---
// Internals
//---

/* _fygue_flash_hw_detect(): detect flash hardware information
 *
 * notes
 *   we cannot easily detect the exact geometry of the flash for the Fugue
 *   FS. For now, simply pseudo-detect hardware information and setup
 *   hardcoded information */
// TODO: support CRC check and try each flash sector until one work ?
// TODO: cluster integrity check seems change between device (?)
int _fygue_flash_hw_detect(struct fygue_flash *flash)
{
    switch (gint[HWCALC])
    {
        case HWCALC_FXCP400:
            flash->geometry.phy_start = 0x01a20000;
            flash->geometry.fsector_count = 0xf2;
            flash->geometry.fcluster_per_fsector = 32;
            flash->integrity = FYGUE_FLASH_INTEGRITY_PARTIAL;
            break;
        case HWCALC_G35PE2:
            flash->geometry.phy_start = 0x003f0000;
            flash->geometry.fsector_count = 0x41;
            flash->geometry.fcluster_per_fsector = 16;
            flash->integrity = FYGUE_FLASH_INTEGRITY_PARTIAL;
            break;
        case HWCALC_FXCG50:
            flash->geometry.phy_start = 0x00c80000;
            flash->geometry.fsector_count = 0x9c;
            flash->geometry.fcluster_per_fsector = 32;
            flash->integrity = FYGUE_FLASH_INTEGRITY_FULL;
            break;
        case HWCALC_FXCG100:
            flash->geometry.phy_start = 0x00bc0000;
            flash->geometry.fsector_count = 0x26;
            flash->geometry.fcluster_per_fsector = 32;
            flash->integrity = FYGUE_FLASH_INTEGRITY_PARTIAL;
            break;
        default:
            errno = ENOTSUP;
            return -1;
    }
    flash->geometry.fcluster_count = (
        flash->geometry.fsector_count * flash->geometry.fcluster_per_fsector
    );
    flash->geometry.fsector_size = (
        flash->geometry.fcluster_per_fsector * (8 * 512)
    );
    flash->geometry.size = (
        flash->geometry.fsector_count * flash->geometry.fsector_size
    );
    flash->geometry.phy_end  = (
        flash->geometry.phy_start + flash->geometry.size
    );
    return 0;
}

//---
// Public
//---

/* fygue_flash_initialize(): init flash abstraction */
int fygue_flash_initialize(struct fygue_flash *flash)
{
    if (flash == NULL)
        return -1;
    memset(flash, 0x00, sizeof(struct fygue_flash));
    if (_fygue_flash_hw_detect(flash) != 0)
        return -2;
    int rc = fygue_flash_cmap_init(flash, &(flash->cmap));
    if (rc != 0)
        return rc;
    return 0;
}
