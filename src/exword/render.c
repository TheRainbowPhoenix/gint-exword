#include <gint/display.h>
#include <gint/drivers/r61523.h>

#ifndef GINT_EXWORD_VRAM_ADDRESS
#define GINT_EXWORD_VRAM_ADDRESS 0xac200000
#endif

uint16_t *gint_vram = (void *)GINT_EXWORD_VRAM_ADDRESS;

void dgetvram(uint16_t **main, uint16_t **secondary)
{
    if(main) *main = gint_vram;
    if(secondary) *secondary = gint_vram;
}

void dsetvram(uint16_t *main, uint16_t *secondary)
{
    if(main || secondary) gint_vram = main ? main : secondary;
}

void dupdate_rect(int x1, int y1, int x2, int y2)
{
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 >= DWIDTH) x2 = DWIDTH-1;
    if(y2 >= DHEIGHT) y2 = DHEIGHT-1;
    if(x1 > x2 || y1 > y2) return;
    r61523_display_rect(gint_vram, x1, x2, y1, y2);
}
