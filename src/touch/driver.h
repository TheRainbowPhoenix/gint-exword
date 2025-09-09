#ifndef GINT_TOUCH_DRIVER_H
#define GINT_TOUCH_DRIVER_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>
#include <gint/keyboard.h>
#include <gint/touch.h>
#include <gint/drivers/touch.h>

#if GINT_HW_CP

/* _touch_drv_info() - internal driver information */
struct _touch_drv_info
{
    touch_calibration_t calibration;
    key_event_t prev_evt;
    struct {
        bool prev_is_dual;
    } adinfo;
};

/* Gets raw data from I2C command 0x84. This function is *NOT REENTRANT* and
   must have only one user (from the scheduled input tick). */
extern void touch_adconv_get_raw(struct _touch_adraw *adraw);

/* touch_adconv_get_conv() - perform the raw conversion */
extern void touch_adconv_get_conv(
    struct _touch_adconv *adconv,
    struct _touch_adraw *adraw);

/* touch_adconv_get_dots() - generate dots information */
extern void touch_adconv_get_dots(
    struct _touch_addots *dots,
    struct _touch_adconv *adconv);

#endif /* GINT_HW_CP */

#ifdef __cplusplus
}
#endif

#endif /* GINT_TOUCH_DRIVER_H */
