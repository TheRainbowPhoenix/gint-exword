#ifndef GINT_TOUCH_DRIVER_H
#define GINT_TOUCH_DRIVER_H 1

#include <gint/keyboard.h>
#include <gint/touch.h>

/* _touch_drv_info() - internal driver information */
struct _touch_drv_info
{
    touch_calibration_t calibration;
    key_event_t prev_evt;
    struct {
        bool prev_is_dual;
    } adinfo;
};

#endif /* GINT_TOUCH_DRIVER_H */
