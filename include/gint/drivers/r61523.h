//---
//  gint:drivers:r61523 - Reneses R61523 driver
//
//  This driver is used to control the 16-bit color LCD of the fx-CP 400.
//---

#ifndef GINT_DRIVERS_R61523
#define GINT_DRIVERS_R61523

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

enum {
    /* Normal/Partial mode (2^16 color) */
    R61523_NORMAL,
    /* Idle mode (2^3 color) */
    R61523_IDLE
};

enum {
    /* Frame inversion waveform (battery saver) */
    R61523_FRAME,
    /* Line inversion waveform (better display quality) */
    R61523_LINE
};

/* r61523_display(): Update the entire display (320x528) */
void r61523_display(uint16_t *vram);

/* r61523_display_rect(): Update a rectangle (both bounds included). */
void r61523_display_rect(
    uint16_t *vram, int xmin, int xmax, int ymin, int ymax);

/* r61523_exit_idle_mode() and r61523_enter_idle_mode(): Toggle idle mode */
void r61523_exit_idle_mode(void);
void r61523_enter_idle_mode(void);

/* r61523_get_power_mode(): Get current power mode */
uint16_t r61523_get_power_mode(void);

/* r61523_get_backlight() and r61523_set_backlight(): Backlight settings

   @EN2         Backlight high bit
   @level       Brightness level (0-255)
   @PWM_div     PWM frequency division
   @dimming     Toggle dimming function */
void r61523_get_backlight(bool *EN2, int *level, int *PWM_div, bool *dimming);
void r61523_set_backlight(bool EN2, int level, int PWM_div, bool dimming);

/* r61523_get_display_timing() and r61523_set_display_timing(): Display timing settings

   @mode        Normal/Partial or Idle mode
   @waveform    Frame or Line waveform inversion
   @CPL         Clocks per line (18-63)
   @BP @FP      Number of back/front porch lines (4-128) */
void r61523_get_display_timing(bool mode, bool *waveform, int *CPL, int *BP, int *FP);
void r61523_set_display_timing(bool mode, bool waveform, int CPL, int BP, int FP);

/* r61523_win_set(): Set the display window */
void r61523_win_set(int x1, int x2, int y1, int y2);

/* r61523_set_pixel(): Write a pixel directly to DD (slow) */
void r61523_set_pixel(int x, int y, int color);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_R61523 */
