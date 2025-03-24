//---
// gint:touch   - touch-screen driver API
//---
#ifndef GINT_TOUCH_H
#define GINT_TOUCH_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>

#if GINT_HW_CP

/* touch_calib - tounch-screen calibration information */
typedef struct {
    int x_base;
    int x_div;
    int y_base;
    int y_div;
    int dual_debounce_frame;
    int dual_sensi_entry;
    int dual_sensi_leave;
} touch_calibration_t;

/* touch_calib_get() - get calibration information */
extern int touch_calib_get(touch_calibration_t *calib);

/* touch_calib_set() - set calibration information */
extern int touch_calib_set(touch_calibration_t *calib);

// low-level API

#include <gint/keyboard.h>

/* touch_next_event() - get the next touchscreen event */
extern key_event_t touch_next_event(void);

#endif /* GINT_HW_CP */

#ifdef __cplusplus
}
#endif

#endif /* GINT_TOUCH_H */
