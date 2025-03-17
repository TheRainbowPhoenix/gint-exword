//---
// gint:touch   - touchscreen driver
//----
#include <string.h>

#include <gint/touch.h>

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

// low-level API

/* touch_next_event() - get the next touchscreen event */
key_event_t touch_next_event(void)
{
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
