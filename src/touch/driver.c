//---
// gint:touch:driver    - touch-screen driver declaration
//---
#include <string.h>

#include <gint/drivers.h>
#include <gint/drivers/states.h>

#include "./driver.h"
#include "./i2c.h"

//---
// Internals
//---

/* __touch_drv_info - internal driver information */
extern struct _touch_drv_info __touch_drv_info;

/* _touch_configure() - configure touch-screen */
static void _touch_configure(void)
{
    volatile uint16_t *IO_PRCR = (void*)0xa405011c;

    i2c_configure();
    *(IO_PRCR) = (*(IO_PRCR) & 0xf3ff) | 0xc00;
    memset(&__touch_drv_info, 0x00, sizeof(struct _touch_drv_info));
    __touch_drv_info.prev_evt.type = KEYEV_NONE;
    __touch_drv_info.prev_evt.x = 0xffff;
    __touch_drv_info.prev_evt.y = 0xffff;
    __touch_drv_info.calibration.x_mul = 0x20b;
    __touch_drv_info.calibration.x_div = 0x9b6;
    __touch_drv_info.calibration.y_mul = 0x0f4;
    __touch_drv_info.calibration.y_div = 0x66f;
    __touch_drv_info.calibration.dual_debounce_frame = 0;
    __touch_drv_info.calibration.dual_sensi_entry = 0x18;
    __touch_drv_info.calibration.dual_sensi_leave = 0x24;
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

/* _touch_unbin() - unbind from gint to casio */
static void _touch_unbin(void)
{
    i2c_unbin();
}

/* _touch_funbin() - funbind from casio to gint */
static void _touch_funbin(void)
{
    i2c_funbin();
}

//---
// Public
//---

/* __touch_drv_info - internal driver information */
struct _touch_drv_info __touch_drv_info;

/* drv_touch - touch-screen driver declaration */
gint_driver_t drv_touch = {
    .name       = "TOUCH",
    .configure  = _touch_configure,
    .hsave      = (void *)_touch_hsave,
    .hrestore   = (void *)_touch_hrestore,
    .hpowered   = _touch_hpowered,
    .hpoweron   = _touch_hpoweron,
    .hpoweroff  = _touch_hpoweroff,
    .unbind     = _touch_unbin,
    .funbind    = _touch_funbin,
    .state_size = sizeof(touch_state_t),
};
GINT_DECLARE_DRIVER(16, drv_touch);
