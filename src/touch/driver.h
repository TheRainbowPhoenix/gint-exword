#ifndef GINT_TOUCH_DRIVER_H
#define GINT_TOUCH_DRIVER_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/keyboard.h>
#include <gint/touch.h>
#include <gint/config.h>

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

#endif /* GINT_HW_CP */

#ifdef __cplusplus
}
#endif

#endif /* GINT_TOUCH_DRIVER_H */
