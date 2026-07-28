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
	/* Send data through the DMA, return early (triple-buffering) */
	R61523_DMA,
	/* Send data through DMA, wait to return (no interrupts) */
	R61523_DMA_WAIT,
	/* Send data through CPU (slow!) */
	R61523_CPU,
};

/* r61523_display(): Send an image to the display

   This function sends [height] lines of the provided [vram] starting from line
   [start] and going down 320 pixels each line. Three methods are avaiable, the
   default is to use R61523_DMA which is what you almost always want.

   @vram    Source VRAM with a stride of 320*2 bytes
   @start   First line to send
   @height  Number of lines to send
   @method  Transfer method, see above */
void r61523_display(uint16_t *vram, int start, int height, int method);

/* r61523_display_rect(): Send a rectangular section of VRAM to the display

   This function updates the rectangle going from (xmin,ymin) to (xmax,ymax) in
   the VRAM to the display (both ends included). This can be faster than a full
   dupdate() or a striped r61523_display() depending on the situation. However,
   because the source VRAM is not contiguous, the transfer is only possible by
   CPU, so this is only interesting for small regions.

   @vram        Source VRAM with a stride of 320*2 bytes
   @xmin @xmax  Horizontal range to be updated (both included)
   @ymin @ymax  Vertical range to be updated (both included) */
void r61523_display_rect(uint16_t *vram, int xmin, int xmax, int ymin,
   int ymax);

/* r61523_start_frame(): Prepare the display for a region update

   This function sets up the display driver to receive graphics data to update
   the rectangle going from (xmin,ymin) to (xmax,ymax) (both included). This is
   the initial step of r61523_display(), which is normally followed by writing
   all the data to 0xb4000000.

   In order to write with the DMA, it is necessary to write the full horizontal
   range and select ymin and ymax to be congruent to 0 and 3 modulo 4.

   This function can be used to implement additional display driver update
   methods or alternate rendering pipelines. */
void r61523_start_frame(int xmin, int xmax, int ymin, int ymax);

/* r61523_win_set(): Set the display window */
void r61523_win_set(int x1, int x2, int y1, int y2);

/* r61523_set_pixel(): Write a pixel directly to DD (slow) */
void r61523_set_pixel(int x, int y, int color);

//---
// Low-level functions
//---

/* r61523_get(): Read the value of an R61523 register
   This is provided for testing and if you know what you're doing. */
uint16_t r61523_get(int ID);

/* r61523_set(): Write the value of an R61523 register
   This is provided for testing and if you know what you're doing. */
void r61523_set(int ID, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_R61523 */
