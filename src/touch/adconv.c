//---
// gint:touchscreen:adconv  - 0x84 register data conversion
//---
#include <gint/defs/util.h>

#include "./adconv.h"
#include "./i2c.h"

//---
// Internals
//---

/* _adconv_check_dual() - check if it is dual or single */
static int _adconv_check_dual(
    struct _touch_adconv *adconv,
    struct _touch_adraw *adraw
) {
    static int _prev_type = 0;
    int x2;
    int y2;
    int z2;
    int val;

    x2 = ((int)((uint)(adraw->x2) >> 6)) + ((adraw->x2 & 1) * -0x400);
    y2 = ((int)((uint)(adraw->y2) >> 6)) + ((adraw->y2 & 1) * -0x400);
    z2 = ((int)((uint)(adraw->z2) >> 4)) + ((adraw->z2 & 1) * -0x1000);
    adconv->x2 = x2;
    adconv->y2 = y2;
    adconv->z2 = z2;
    val = (_prev_type == 0) ? 0x18 : 0x12;
    _prev_type = ((abs(z2) >= val) || (max(abs(x2),abs(y2)) >= val));
    return _prev_type;
}

//---
// Public
//---

/* touchscreen_adconv_get_raw() - read 0x84 register using I2C */
int touchscreen_adconv_get_raw(struct _touch_adraw *adraw)
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
        return 0;
    }
    i2c_reg_read(0x84, adraw, 16);
    return 1;
}

/* touchscreen_adconv_convert() - perform the raw conversion */
int touchscreen_adconv_get_conv(
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

/* touchscreen_adconv_get_dots() - generate dots information */
int touchscreen_adconv_get_dots(
    struct _touch_addots *dots,
    struct _touch_adconv *adconv,
    int type
) {
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
            dots->x1 = ((adconv->x1 - 0x1b8) * 0x100) / 0x9e5;
            dots->y1 = ((adconv->y1 - 0x104) * 0x100) / 0x660;
            dots->z1 = adconv->z1;
            dots->x2 = 0;
            dots->y2 = 0;
            dots->z2 = 0;
            break;
        case 2:
            dots->type = TS_DOTS_TYPE_DUAL;
            dots->x1 = ((adconv->x1 - 0x1b8) * 0x100) / 0x9e5;
            dots->y1 = ((adconv->y1 - 0x104) * 0x100) / 0x660;
            dots->z1 = adconv->z1;
            dots->x2 = ((adconv->x2 - 0x1b8) * 0x100) / 0x9e5;
            dots->y2 = ((adconv->y2 - 0x104) * 0x100) / 0x660;
            dots->z2 = adconv->z2;
            break;
    }
    return type;
}
