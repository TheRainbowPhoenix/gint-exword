//---
// gint:touch   - touchscreen driver (high-level)
//----
#include <string.h>

#include <gint/touch.h>
#include <gint/cpu.h>

#include "./i2c.h"
#include "./adconv.h"
#include "./driver.h"

//---
// Internals
//---

/* __touch_drv_info - internal driver information */
extern struct _touch_drv_info __touch_drv_info;

//---
// Public
//---

// user-API

/* touch_calib_get() - get calibration information */
int touch_calib_get(touch_calib *calib)
{
    if (calib == NULL)
        return -1;
    memcpy(calib, &__touch_drv_info.calibration, sizeof(touch_calib));
    return 0;
}

/* touch_calib_set() - set calibration information */
int touch_calib_set(touch_calib *calib)
{
    if (calib == NULL)
        return -1;
    memcpy(&__touch_drv_info.calibration, calib, sizeof(touch_calib));
    return 0;
}

// low-level API

/* touch_next_event() - get the next touchscreen event */
key_event_t touch_next_event(void)
{
    struct _touch_adconv adconv;
    struct _touch_addots addots;
    struct _touch_adraw adraw;
    key_event_t evt;
    int type;

    evt.type = KEYEV_TOUCH_RELEASE;
    type = touchscreen_adconv_get_raw(&adraw);
    if (type != 0)
    {
        type = touchscreen_adconv_get_conv(&adconv, &adraw, type);
        type = touchscreen_adconv_get_dots(&addots, &adconv, type);
        if (type == 1)
        {
            evt.type = KEYEV_TOUCH_DRAG;
            evt.x = addots.x1;
            evt.y = addots.y1;
        }
    }
    cpu_atomic_start();
    if (evt.type == KEYEV_TOUCH_DRAG) {
        if (
            (__touch_drv_info.prev_evt.type == KEYEV_NONE) ||
            (__touch_drv_info.prev_evt.type == KEYEV_TOUCH_RELEASE)
        ) {
            evt.type = KEYEV_TOUCH_PRESSED;
        }
    }
    if (evt.type == KEYEV_TOUCH_RELEASE) {
        if (
            (__touch_drv_info.prev_evt.type == KEYEV_NONE) ||
            (__touch_drv_info.prev_evt.type == KEYEV_TOUCH_RELEASE)
        ) {
            evt.type = KEYEV_NONE;
        }
    }
    __touch_drv_info.prev_evt.type = evt.type;
    __touch_drv_info.prev_evt.x = evt.x;
    __touch_drv_info.prev_evt.y = evt.y;
    cpu_atomic_end();
    return evt;
}
