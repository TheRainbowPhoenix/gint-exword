//---
// gint:touch   - touchscreen driver
//----
#include <string.h>

#include <gint/drivers/states.h>
#include <gint/drivers.h>
#include <gint/touch.h>

#include "./i2c.h"
#include "./adconv.h"

//---
// Internals
//---

touch_calib __ts_calib;

//---
// Public
//---

// user-API

/* touch_calib_get() - get calibration information */
int touch_calib_get(touch_calib *calib)
{
    if (calib == NULL)
        return -1;
    memcpy(calib, &__ts_calib, sizeof(touch_calib));
    return 0;
}

/* touch_calib_set() - set calibration information */
int touch_calib_set(touch_calib *calib)
{
    if (calib == NULL)
        return -1;
    memcpy(&__ts_calib, calib, sizeof(touch_calib));
    return 0;
}

/* touch_get_dots() - get dots information */
int touch_get_dots(struct _touch_addots *dots)
{
    struct _touch_adraw adraw;
    struct _touch_adconv adconv;
    int type;

    type = touchscreen_adconv_get_raw(&adraw);
    type = touchscreen_adconv_get_conv(&adconv, &adraw, type);
    type = touchscreen_adconv_get_dots(dots, &adconv, type);
    return type;
}

// low-level API

/* touch_next_event() - get the next touchscreen event */
key_event_t touch_next_event(void)
{
    key_event_t evt = { 0 };
    evt.type = KEYEV_NONE;
    return evt;
#if 0
    struct _ts_adraw adraw;
    struct _ts_adconv adconv;
    int type;

    type = touchscreen_adconv_get_raw(&adraw);
    type = touchscreen_adconv_get_conv(&adconv, &adraw, type);
    type = touchscreen_adconv_get_dots(dots, &adconv, type);
    return type;
#endif
}

//---
// Driver and state
//---

/* _touch_configure() - configure touch-screen */
static void _touch_configure(void)
{
    volatile uint16_t *IO_PRCR = (void*)0xa405011c;

    i2c_configure();
    *(IO_PRCR) = (*(IO_PRCR) & 0xf3ff) | 0xc00;
    __ts_calib.x_mul = 0x20b;
    __ts_calib.x_div = 0x9b6;
    __ts_calib.y_mul = 0x0f4;
    __ts_calib.y_div = 0x66f;
    __ts_calib.dual_debounce_frame = 0;
    __ts_calib.dual_sensi_entry = 0x18;
    __ts_calib.dual_sensi_leave = 0x24;
}

/* _touch_hsave() - save hardware state */
static void _touch_hsave(touch_state_t *state)
{
    volatile uint16_t *IO_PRCR = (void*)0xa405011c;

    i2c_hsave(state);
    state->PRCR = *(IO_PRCR);
}

/* _touch_hrestore() - restore hardware state */
static void _touch_hrestore(touch_state_t *state)
{
    volatile uint16_t *IO_PRCR = (void*)0xa405011c;

    *(IO_PRCR) = state->PRCR;
    i2c_hrestore(state);
}

/* _touch_hpowered() - check if the module is powered */
static bool _touch_hpowered(void)
{
    return i2c_hpowered();
}

/* _touch_hpoweron() - power on the module */
static void _touch_hpoweron(void)
{
    i2c_hpoweron();
}

/* _touch_hpoweroff() - power off the module */
static void _touch_hpoweroff(void)
{
    i2c_hpoweroff();
}

gint_driver_t drv_touch = {
    .name       = "TOUCH",
    .configure  = _touch_configure,
    .hsave      = (void *)_touch_hsave,
    .hrestore   = (void *)_touch_hrestore,
    .hpowered   = _touch_hpowered,
    .hpoweron   = _touch_hpoweron,
    .hpoweroff  = _touch_hpoweroff,
    .state_size = sizeof(touch_state_t),
};
GINT_DECLARE_DRIVER(16, drv_touch);
