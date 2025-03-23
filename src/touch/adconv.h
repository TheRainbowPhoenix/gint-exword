#ifndef GINT_TOUCH_ADCONV_H
#define GINT_TOUCH_ADCONV_H 1

#include <gint/defs/types.h>

//---
// Internals
//---

/* _touch_adraw - raw 0x84 register information */
struct _touch_adraw
{
    uint16_t x1;
    uint16_t y1;
    uint16_t z1;
    uint16_t gh;
    uint16_t x2;
    uint16_t y2;
    uint16_t z2;
    uint16_t dm;
};

/* _touch_adconv - post-conversion raw 0x84 register information */
struct _touch_adconv
{
    int x1;
    int y1;
    int x2;
    int y2;
    int z1;
    int z2;
    uint16_t gh;
    uint16_t dm;
};

/* _touch_addots_type - type of dots */
enum _touch_addots_type
{
    TS_DOTS_TYPE_OFF    = 0,
    TS_DOTS_TYPE_SINGLE = 1,
    TS_DOTS_TYPE_DUAL   = 2,
};

/* _touch_addots - touchscreen dots information */
struct _touch_addots
{
    enum _touch_addots_type type;
    int x1;
    int y1;
    int z1;
    int x2;
    int y2;
    int z2;
};

//---
// Public
//---

/* touch_adconv_get_raw() - read 0x84 register using I2C */
extern int touch_adconv_get_raw(struct _touch_adraw *adraw);

/* touch_adconv_get_conv() - perform the raw conversion */
extern int touch_adconv_get_conv(
    struct _touch_adconv *adconv,
    struct _touch_adraw *adraw,
    int type
);

/* touch_adconv_get_dots() - generate dots information */
extern int touch_adconv_get_dots(
    struct _touch_addots *dots,
    struct _touch_adconv *adconv,
    int type
);

#endif /* GINT_TOUCH_ADCONV_H */
