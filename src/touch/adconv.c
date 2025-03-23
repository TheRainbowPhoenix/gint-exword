//---
// gint:touch:adconv  - 0x84 register data conversion
//---
#include <stdlib.h>

#include <gint/defs/util.h>
#include <gint/cpu.h>

#include "./adconv.h"
#include "./i2c.h"
#include "./driver.h"

//---
// Internals
//---

/* __touch_drv_info - global driver information */
extern struct _touch_drv_info __touch_drv_info;

/* _adconv_check_dual() - check if it is dual or single */
static int _adconv_check_dual(
    struct _touch_adconv *adconv,
    struct _touch_adraw *adraw
) {
    extern struct _touch_drv_info __touch_drv_info;
    bool is_dual;
    int x2;
    int y2;
    int z2;
    int val;

    cpu_atomic_start();
    x2 = ((int)((uint)(adraw->x2) >> 6)) + ((adraw->x2 & 1) * -0x400);
    y2 = ((int)((uint)(adraw->y2) >> 6)) + ((adraw->y2 & 1) * -0x400);
    z2 = ((int)((uint)(adraw->z2) >> 4)) + ((adraw->z2 & 1) * -0x1000);
    adconv->x2 = x2;
    adconv->y2 = y2;
    adconv->z2 = z2;
    val = __touch_drv_info.calibration.dual_sensi_entry;
    if (__touch_drv_info.adinfo.prev_is_dual)
        val = __touch_drv_info.calibration.dual_sensi_leave;
    is_dual = ((abs(z2) >= val) || (max(abs(x2),abs(y2)) >= val));
    __touch_drv_info.adinfo.prev_is_dual = is_dual;
    cpu_atomic_end();
    return is_dual;
}

//---
// Public
//---

/* touch_adconv_get_raw() - read 0x84 register using I2C */
int touch_adconv_get_raw(struct _touch_adraw *adraw)
{
    volatile uint8_t *IO_PRDR = (void*)0xa405013c;

    if (((*IO_PRDR) & 0x20) != 0)
    {
        adraw->x1 = 0;
        adraw->y1 = 0;
        adraw->z1 = 0;
        adraw->x2 = 0;
        adraw->y2 = 0;
        adraw->z2 = 0;
        adraw->gh = 0;
        cpu_atomic_start();
        __touch_drv_info.adinfo.prev_is_dual = false;
        cpu_atomic_end();
        return 0;
    }
    i2c_reg_read(0x84, adraw, 16);
    return 1;
}

/* touch_adconv_convert() - perform the raw conversion */
int touch_adconv_get_conv(
    struct _touch_adconv *adconv,
    struct _touch_adraw *adraw,
    int type
) {
    if (type == 0)
        return 0;
    adconv->x1 = adraw->x1 >> 4;
    adconv->y1 = adraw->y1 >> 4;
    adconv->z1 = adraw->z1 >> 4;
    adconv->gh = adraw->gh >> 4;
    adconv->dm = adraw->dm >> 6;
    if (_adconv_check_dual(adconv, adraw) == 0)
        return 1;
    return 2;
}

/* touch_adconv_get_dots() - generate dots information */
int touch_adconv_get_dots(
    struct _touch_addots *dots,
    struct _touch_adconv *adconv,
    int type
) {
    int x_div;
    int x_base;
    int y_div;
    int y_base;

    cpu_atomic_start();
    x_div = __touch_drv_info.calibration.x_div;
    x_base = __touch_drv_info.calibration.x_base;
    y_div = __touch_drv_info.calibration.y_div;
    y_base = __touch_drv_info.calibration.y_base;
    switch (type)
    {
        case 0:
            dots->type = TS_DOTS_TYPE_OFF;
            dots->x1 = 0;
            dots->y1 = 0;
            dots->z1 = 0;
            dots->x2 = 0;
            dots->y2 = 0;
            dots->z2 = 0;
            break;
        case 1:
            dots->type = TS_DOTS_TYPE_SINGLE;
            dots->x1 = ((adconv->x1 - x_base) * 0x100) / x_div;
            dots->y1 = ((adconv->y1 - y_base) * 0x100) / y_div;
            dots->z1 = adconv->z1;
            dots->x2 = 0;
            dots->y2 = 0;
            dots->z2 = 0;
            break;
        case 2:
            dots->type = TS_DOTS_TYPE_DUAL;
            dots->x1 = ((adconv->x1 - x_base) * 0x100) / x_div;
            dots->y1 = ((adconv->y1 - y_base) * 0x100) / y_div;
            dots->z1 = adconv->z1;
            dots->x2 = ((adconv->x2 - x_base) * 0x100) / x_div;
            dots->y2 = ((adconv->y2 - y_base) * 0x100) / y_div;
            dots->z2 = adconv->z2;
            break;
    }
    cpu_atomic_end();
    return type;
}
