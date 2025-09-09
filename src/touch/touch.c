//---
// gint:touch   - touch driver (high-level)
//----
#include <string.h>
#include <stdlib.h>

#include <gint/config.h>
#include <gint/defs/util.h>
#include <gint/touch.h>
#include <gint/cpu.h>

#if GINT_HW_CP

#include "./i2c.h"
#include "./driver.h"

/* __touch_drv_info - internal driver information */
extern struct _touch_drv_info __touch_drv_info;

// TODO[touch]: Get rid of global last_cong by making raw -> conv stateless
static struct _touch_adraw last_raw;
static struct _touch_adconv last_conv;
static struct _touch_addots last_dots;

//---
// Internals
//---

void touch_adconv_get_raw(struct _touch_adraw *adraw)
{
    volatile uint8_t *IO_PRDR = (void*)0xa405013c;
    memset(adraw, 0, sizeof *adraw);

    if ((*IO_PRDR & 0x20) == 0) {
        i2c_reg_read(0x84, adraw, 16);
        adraw->has_data = true;
    }
}

void touch_adconv_get_conv(
    struct _touch_adconv *adconv,
    struct _touch_adraw *adraw)
{
    if (!adraw->has_data) {
        memset(adconv, 0, sizeof *adconv);
        cpu_atomic_start();
        __touch_drv_info.adinfo.prev_is_dual = false;
        cpu_atomic_end();
        return;
    }

    adconv->x1 = adraw->x1 >> 4;
    adconv->y1 = adraw->y1 >> 4;
    adconv->z1 = adraw->z1 >> 4;
    adconv->x2 = ((int)((uint)(adraw->x2) >> 6)) + ((adraw->x2 & 1) * -0x400);
    adconv->y2 = ((int)((uint)(adraw->y2) >> 6)) + ((adraw->y2 & 1) * -0x400);
    adconv->z2 = ((int)((uint)(adraw->z2) >> 4)) + ((adraw->z2 & 1) * -0x1000);
    adconv->gh = adraw->gh >> 4;
    adconv->dm = adraw->dm >> 6;

    cpu_atomic_start();
    int dual_threshold = __touch_drv_info.calibration.dual_sensi_entry;
    if (__touch_drv_info.adinfo.prev_is_dual)
        dual_threshold = __touch_drv_info.calibration.dual_sensi_leave;
    bool is_dual = (abs(adconv->z2) >= dual_threshold ||
                    max(abs(adconv->x2), abs(adconv->y2)) >= dual_threshold);
    __touch_drv_info.adinfo.prev_is_dual = is_dual;
    cpu_atomic_end();

    adconv->touches = is_dual ? 2 : 1;
}

void touch_adconv_get_dots(
    struct _touch_addots *dots,
    struct _touch_adconv *adconv)
{
    int x_div;
    int x_base;
    int y_div;
    int y_base;

    cpu_atomic_start();
    x_div = __touch_drv_info.calibration.x_div;
    x_base = __touch_drv_info.calibration.x_base;
    y_div = __touch_drv_info.calibration.y_div;
    y_base = __touch_drv_info.calibration.y_base;
    cpu_atomic_end();

    dots->touches = adconv->touches;
    switch (adconv->touches)
    {
        case 0:
            dots->x1 = 0;
            dots->y1 = 0;
            dots->z1 = 0;
            dots->x2 = 0;
            dots->y2 = 0;
            dots->z2 = 0;
            break;
        case 1:
            dots->x1 = ((adconv->x1 - x_base) * 0x100) / x_div;
            dots->y1 = ((adconv->y1 - y_base) * 0x100) / y_div;
            dots->z1 = adconv->z1;
            dots->x2 = 0;
            dots->y2 = 0;
            dots->z2 = 0;
            break;
        case 2: {
            int xmin = ((adconv->x1 - x_base) * 0x100) / x_div;
            int ymin = ((adconv->y1 - y_base) * 0x100) / y_div;
            // x2 end-to-end: ~185 (changes along the height)
            // y2 end-to-end: 470
            // x2 *= 1.64
            // y2 *= 0.96
            int xmax = xmin + adconv->x2 * 0x1ba / 0x100;
            int ymax = ymin + adconv->y2 * 0x11f / 0x100;
            dots->z1 = adconv->z1;
            dots->z2 = adconv->z2;

            if(dots->z2 >= 0) {
                dots->x1 = xmin;
                dots->y1 = ymin;
                dots->x2 = xmax;
                dots->y2 = ymax;
            }
            else {
                dots->x1 = xmin;
                dots->y1 = ymax;
                dots->x2 = xmax;
                dots->y2 = ymin;
            }
            dots->z1 = adconv->x2;
            dots->z2 = adconv->y2;
            break;
        }
    }
}

//---
// Public
//---

// user-API

/* touch_calib_get() - get calibration information */
int touch_calib_get(touch_calibration_t *calib)
{
    if (calib == NULL)
        return -1;
    memcpy(calib, &__touch_drv_info.calibration, sizeof(*calib));
    return 0;
}

int touch_calib_get_os(touch_calibration_t *calib)
{
    void *os_version = (void *)0x80020020;
    if(!memcmp(os_version, "02.01.200", 9) ||
       !memcmp(os_version, "02.01.700", 9)) {
        int const *os_calibration = (void *)0x8c1bea50;
        calib->x_base = os_calibration[0];
        calib->y_base = os_calibration[1];
        calib->x_div = os_calibration[2];
        calib->y_div = os_calibration[3];
        return 0;
    }

    return -1;
}

/* touch_calib_set() - set calibration information */
int touch_calib_set(touch_calibration_t *calib)
{
    if (calib == NULL)
        return -1;
    memcpy(&__touch_drv_info.calibration, calib, sizeof(*calib));
    return 0;
}

// low-level API

static key_event_t make_event(struct _touch_addots const *dots)
{
    key_event_t evt;
    evt.type = KEYEV_NONE;

    cpu_atomic_start();
    int prev_type = __touch_drv_info.prev_evt.type;
    int prev_x = __touch_drv_info.prev_evt.x;
    int prev_y = __touch_drv_info.prev_evt.y;

    // TODO: Handle dual touch here
    if(dots->touches != 1) {
        if(prev_type != KEYEV_TOUCH_UP && prev_type != KEYEV_NONE)
            evt.type = KEYEV_TOUCH_UP;
        evt.x = prev_x;
        evt.y = prev_y;
    }
    else {
        if(prev_type == KEYEV_TOUCH_UP || prev_type == KEYEV_NONE)
            evt.type = KEYEV_TOUCH_DOWN;
        /* Don't emit successive DRAG events at the same location */
        else if(dots->x1 != prev_x || dots->y1 != prev_y)
            evt.type = KEYEV_TOUCH_DRAG;

        if(evt.type != KEYEV_NONE) {
            evt.x = dots->x1;
            evt.y = dots->y1;
        }
    }

    if(evt.type != KEYEV_NONE)
        __touch_drv_info.prev_evt = evt;
    cpu_atomic_end();
    return evt;
}

key_event_t touch_scan(void)
{
    touch_adconv_get_raw(&last_raw);
    touch_adconv_get_conv(&last_conv, &last_raw);
    touch_adconv_get_dots(&last_dots, &last_conv);
    return make_event(&last_dots);
}

void touch_get_last_scan(struct _touch_adraw *raw,
    struct _touch_adconv *conv, struct _touch_addots *dots)
{
    if(raw)
        *raw = last_raw;
    if(conv)
        *conv = last_conv;
    if(dots)
        *dots = last_dots;
}

#endif /* GINT_HW_CP */
