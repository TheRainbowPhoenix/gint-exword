//---
// gint:touch   - touch-screen driver API
//---
#ifndef GINT_TOUCH_H
#define GINT_TOUCH_H 1

#ifdef __cplusplus
extern "C" {
#endif

/* touch_calib - tounch-screen calibration information */
typedef struct {
    int scan_freq_us;
    int x_mul;
    int x_div;
    int y_mul;
    int y_div;
    int dual_debounce_frame;
    int dual_sensi_entry;
    int dual_sensi_leave;
} touch_calib;

/* touch_calib_get() - get calibration information */
extern int touch_calib_get(touch_calib *calib);

/* touch_calib_set() - set calibration information */
extern int touch_calib_set(touch_calib *calib);

// low-level API

#include <gint/keyboard.h>

/* touch_next_event() - get the next touchscreen event */
extern key_event_t touch_next_event(void);

#ifdef __cplusplus
}
#endif

#endif /* GINT_TOUCH_H */
