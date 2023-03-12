#include <gint/display.h>
#include <gint/kmalloc.h>
#include <gint/config.h>
#include <gint/drivers/r61524.h>
#include "../render-fx/render-fx.h"

/* The destination VRAM for upscaling + centering + conversion B&W --> 16bits RGB565  */
uint16_t *cg_vram; // set to 0 at initialisation

/* Standard video RAM for fx9860g is 1 bit per pixel */
GALIGNED(32) static uint32_t fx_vram[256];

/* Here is the definition of the VRAM pointer, exposed in <gint/display.h> */
uint32_t *gint_vram = fx_vram;

/* The current rendering mode */
struct rendering_mode const *dmode = NULL;


bool dvram_init( void )
{
	int const MARGIN = 32;

	/* Leave MARGIN bytes on each side of the region; this enables some
	   important optimizations in the image renderer. We also add another
	   32 bytes so we can manually 32-align the region */
	uint32_t region = (uint32_t)kmalloc(DWIDTH*DHEIGHT*2 + MARGIN*2 + 32,
#if !defined(GINT_NO_OS_STACK)
		"_ostk"
#else
		NULL
#endif
	);
	if(region == 0)
		return false;

	/* 32-align the region */
	region = (region + 31) & -32;
	/* Skip a MARGIN */
	region += MARGIN;
	/* Use an uncached address */
	region = (region & 0x1fffffff) | 0xa0000000;

	/* Don't enable triple buffering by default */
	cg_vram = (void *)region;
	return true;
}

void dgetvram(uint16_t **ptr_vram_1, uint16_t **ptr_vram_2)
{
	if(ptr_vram_1) *ptr_vram_1 = &cg_vram;
	if(ptr_vram_2) *ptr_vram_2 = &cg_vram;
}

inline drawupscale( int x, int y, int color )
{
    int u=y*396*3*2;

    uint16_t colorcg;
    
    if (color==C_WHITE) colorcg=0xFFFF;
    else colorcg=0x0000;

    int baseindex = (396*16+6+x+u);          // 16 lines on top/bottom remain black and 6 columns on left/right remain black
    cg_vram[baseindex] = colorcg;
    cg_vram[baseindex+1] = colorcg;
    cg_vram[baseindex+2] = colorcg;

    baseindex+=396;
    cg_vram[baseindex] = colorcg;
    cg_vram[baseindex+1] = colorcg;
    cg_vram[baseindex+2] = colorcg;

    baseindex+=396;
    cg_vram[baseindex] = colorcg;
    cg_vram[baseindex+1] = colorcg;
    cg_vram[baseindex+2] = colorcg;
}

/* dupdate(): Push the video RAM to the display driver */
void dupdate(void)
{

    for( int j=0; j<DHEIGHT; j++ )          // 64 lines
    {
        for( int i=0; i<DWIDTH; i++ )       // 128 column
        {
            drawupscale( i, j, dgetpixel(i,j) );        // really not optimised; just to check if OK
        } 
    }

	r61524_display(cg_vram, 0, 224, R61524_DMA_WAIT );

	gint_call(dupdate_get_hook());
}
__attribute__((alias("dupdate")))
void _WEAK_dupdate(void);