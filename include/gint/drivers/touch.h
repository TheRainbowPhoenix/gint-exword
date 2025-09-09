//---
// gint:drivers:touch - Internals of the touch driver
//---

#ifndef GINT_DRIVERS_TOUCH
#define GINT_DRIVERS_TOUCH

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>
#include <gint/defs/types.h>
#include <gint/keyboard.h>

#if GINT_HW_CP

/* Access to the latest data of each stage of the touch input sequence.
   * First stage is the raw output of the 0x84 command from I2C.
   * Second stage is the same data after adjusting resolution and bit formats,
     then determining then number of touches based on multi-touch parameters.
   * Third stage is converting to screen-space coordinates based on calibration
     parameters. */

/* First stage: raw output of command 0x84. There may not be data at all if the
   input port indicates there are no touches. */
struct _touch_adraw {
    /* Response to command 0x84 */
    u16 x1, y1, z1, gh;
    u16 x2, y2, z2, dm;
    /* Whether data is present (if false, everything is 0 and no touches) */
    bool has_data;
};

/* Second stage: decoded form of command 0x84 output. */
struct _touch_adconv {
    /* Number of touches (0, 1 or 2) */
    int touches;
    int x1, y1;
    int x2, y2;
    int z1, z2;
    u16 gh, dm;
};

/* Third stage: screen-space dots information. */
struct _touch_addots {
    /* Number of touches (0, 1 or 2) */
    int touches;
    /* Valid if touches >= 1 */
    int x1, y1, z1;
    /* Valid if touches >= 2 */
    int x2, y2, z2;
};

/* Run a scanning round and return the next event. This function is not
   reentrant and is already called by the input timer. Don't call it! */
key_event_t touch_scan(void);

/* Get the last captured data from any of the 3 stages. All pointers can be
   NULL; fills the structures that are provided. */
void touch_get_last_scan(struct _touch_adraw *raw,
    struct _touch_adconv *conv, struct _touch_addots *dots);

#endif /* GINT_HW_CP */

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_TOUCH */
