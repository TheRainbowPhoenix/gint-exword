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

/* touch_calib_get_os() - get OS calibration information, if available
   Returns 0 on success, negative number if not available. */
extern int touch_calib_get_os(touch_calibration_t *calib);

/* touch_calib_set() - set calibration information */
extern int touch_calib_set(touch_calibration_t *calib);

// TODO: Functions to query the immediate state of the touch display
// TODO| As per queue? Or real-time?

#endif /* GINT_HW_CP */

#ifdef __cplusplus
}
#endif

#endif /* GINT_TOUCH_H */
