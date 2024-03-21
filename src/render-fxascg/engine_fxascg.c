//---
//	gint:gray:engine - Core gray engine
//---

#include <gint/defs/types.h>
#include <gint/drivers/r61524.h>
#include <gint/gray.h>
#include <gint/display.h>
#include <gint/kmalloc.h>

#include <stdlib.h>

#include "../render/render.h"
#include "../render-fx/render-fx.h"

/* Three additional video RAMS, allocated statically if --static-gray was set
   at configure time, or with malloc() otherwise. */
#ifdef GINT_STATIC_GRAY
GBSS static uint32_t gvrams[1][256];
#endif

uint16_t *cg_vram_gray;

/* two VRAMs: two to draw and two to display */
static uint32_t *vrams[2] = { NULL, NULL };

/* Whether the engine is scheduled to run at the next frame */
static int runs = 0;

extern struct rendering_mode const *dmode;


/* The alternate rendering mode structure used to override d*() */
static struct rendering_mode const gray_mode = {
	.dupdate      = gupdate,
	.dclear       = gclear,
	.drect        = grect,
	.dpixel       = gpixel,
	.dgetpixel    = ggetpixel,
	.gint_dhline  = gint_ghline,
	.gint_dvline  = gint_gvline,
	.dtext_opt    = gtext_opt,
	.dsubimage    = gsubimage,
};
static struct rendering_mode const gray_exit_mode = {
	.dupdate      = gupdate,
	.dclear       = NULL,
	.drect        = NULL,
	.dpixel       = NULL,
	.dgetpixel    = NULL,
	.gint_dhline  = NULL,
	.gint_dvline  = NULL,
	.dtext_opt    = NULL,
	.dsubimage    = NULL,
};

//---
//	Engine control (init/quit and start/stop)
//---

//static int gray_int(void);
static void gray_quit(void);

/* gray_isinit(): Check whether the engine is initialized and ready to run */
static int gray_isinit(void)
{
	//return (vrams[0] && vrams[1] && vrams[2] && vrams[3] && timer >= 0);
	return (vrams[0] && vrams[1]);
}


bool dvram_init_gray( void )
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
	cg_vram_gray = (void *)region;
	return true;
}



/* gray_init(): Initialize the engine
   This is done at startup so that memory can be reserved very early from the
   heap (because not having enough memory is unrecoverable for the engine). */
GCONSTRUCTOR static void gray_init(void)
{
	/* We need four VRAMs. First use the standard monochrome one */
	vrams[0] = gint_vram;

	#ifdef GINT_STATIC_GRAY
	vrams[1] = gvrams[0];
	#else
	vrams[1] = malloc(1024);
	#endif /* GINT_STATIC_GRAY */

	dvram_init_gray();

	/* On failure, release the resources that we obtained */
	if(!gray_isinit()) gray_quit();
}

/* gray_quit(): Free engine resources */
GDESTRUCTOR static void gray_quit(void)
{
	#ifndef GINT_STATIC_GRAY
	if(vrams[1]) free(vrams[1]);
	vrams[1] = NULL;
	#endif /* GINT_STATIC_GRAY */
}

/* gray_start(): Start the gray engine */
static void gray_start(void)
{
	runs = 1;
}

/* gray_stop(): Stop the gray engine */
static void gray_stop(void)
{
	runs = 0;
}

//---
//	Dynamic udpate and rendering mode
//---

/* dgray(): Start or stop the gray engine at the next dupdate() */
int dgray(int mode)
{
	/* Stack of states for the push modes */
	static uint8_t states[32] = { 0 };
	static uint8_t current = 0;

	if(mode == DGRAY_ON)
	{
		if(!gray_isinit()) return 1;

		/* Set the display module's alternate rendering mode to
		   override rendering functions to use their g*() variant */
		if(!dgray_enabled()) dmode = &gray_mode;
	}
	else if(mode == DGRAY_OFF)
	{
		/* Set the mode to a temporary one that only overrides
		   dupdate() so that we can stop the engine next frame */
		if(dgray_enabled()) dmode = &gray_exit_mode;
	}
	else if(mode == DGRAY_PUSH_ON)
	{
		if(current >= 32) return 1;
		states[current++] = dgray_enabled() ? DGRAY_ON : DGRAY_OFF;

		return dgray(DGRAY_ON);
	}
	else if(mode == DGRAY_PUSH_OFF)
	{
		if(current >= 32) return 1;
		states[current++] = dgray_enabled() ? DGRAY_ON : DGRAY_OFF;

		return dgray(DGRAY_OFF);
	}
	else if(mode == DGRAY_POP)
	{
		/* Stay at 0 if the user's push/pop logic is broken */
		if(current > 0) current--;

		/* Switch to previous state */
		return dgray(states[current]);
	}
	else return 1;

	return 0;
}

/* convert the gray scale into RGB565 and draw in the virutal VRAMthe 3x3 pixels upscaled and centered */
void gdrawupscale( int x, int y, int color_fx )
{
    int u=y*396*3;
    int v=x*3;

    uint16_t color_cg;
    
    if (color_fx==C_WHITE) 		  color_cg=0xFFFF;
	else if (color_fx==C_LIGHT)   color_cg=0xAD55;
	else if (color_fx==C_DARK) 	  color_cg=0x528A;
	else 						  color_cg=0x0000;

    int baseindex = (396*16+6+v+u);          // 16 lines on top/bottom remain black and 6 columns on left/right remain black
    cg_vram_gray[baseindex] 	= color_cg;		 // draw 3 pixels side by side
    cg_vram_gray[baseindex+1] 	= color_cg;
    cg_vram_gray[baseindex+2] 	= color_cg;

    baseindex+=396;							 // same for the line below (line #2)
    cg_vram_gray[baseindex] 	= color_cg;
    cg_vram_gray[baseindex+1] 	= color_cg;
    cg_vram_gray[baseindex+2] 	= color_cg;

    baseindex+=396;							 // same for the line below (line #3)
    cg_vram_gray[baseindex] 	= color_cg;
    cg_vram_gray[baseindex+1] 	= color_cg;
    cg_vram_gray[baseindex+2] 	= color_cg;
}


/* gray_int(): Interrupt handler */
//int gray_int(void)
//{
//	//t6k11_display(vrams[st ^ 2], 0, 64, 16);
//	timer_reload(GRAY_TIMER, delays[(st ^ 3) & 1]);
//	
//	st ^= 1;
//
//	return TIMER_CONTINUE;
//}

/* gupdate(): Push the current VRAMs to the screen */
int gupdate(void)
{
	/* At the first gupdate(), start the engine */
	if(dmode == &gray_mode && !runs)
	{
		gray_start();
	}
	/* At the last gupdate(), stop the engine */
	else if(dmode == &gray_exit_mode)
	{
		gray_stop();
		dmode = NULL;
		return 1;
	}


    uint32_t *light, *dark;
	dgray_getscreen(&light, &dark);

    for( int j=0; j<DHEIGHT; j++ )          // 64 lines
    {
        for( int i=0; i<DWIDTH; i++ )       // 128 column
        {
            int offset = (j << 2) + (i >> 5);
            uint32_t mask = 1 << (~i & 31);
            int l = (light[offset] & mask) !=0 ? 1 : 0;
            int d = (dark [offset] & mask) !=0 ? 1 : 0;

            int color_fx = (d << 1) | l;

			gdrawupscale( i, j, color_fx );        // really not optimised; just to check if OK
        } 
    }


    //draw a black layer around the screen
    int base1 = 0;
    int base2 = (64*3+16)*396;
    for(int u = 0; u<16; u++)
    {
        for(int v = 0; v<396; v++)
		{
            cg_vram_gray[base1+v] = 0;
			cg_vram_gray[base2+v] = 0;
		}
		base1+=396;
		base2+=396;
    }

    base1 = 16*396;
    base2 = 16*396+6+128*3;
    for(int u = 0; u<64*3; u++)
    {
        for(int v = 0; v<6; v++)
		{
            cg_vram_gray[base1+v] = 0;
			cg_vram_gray[base2+v] = 0;
		}
		base1+=396;
		base2+=396;
    }

	r61524_display(cg_vram_gray, 0, 224, R61524_DMA_WAIT );

	/* When the engine is running, swap frames */
	return 0;
}

//---
//	Query and configuration functions
//---

/* dgray_enabled(): Check whether gray mode is enabled */
int dgray_enabled(void)
{
	return (dmode == &gray_mode);
}

/* dgray_setdelays(): Set the gray engine delays */
void dgray_setdelays(uint32_t light, uint32_t dark)
{
//	delays[0] = light;
//	delays[1] = dark;
}

/* dgray_getdelays(): Get the gray engine delays */
void dgray_getdelays(uint32_t *light, uint32_t *dark)
{
//	if(light) *light = delays[0];
//	if(dark) *dark = delays[1];
}

/* dgray_getvram(): Get the current VRAM pointers */
void dgray_getvram(uint32_t **light, uint32_t **dark)
{
	if(light) *light = vrams[0];
	if(dark) *dark = vrams[1];
}

/* dgray_getscreen(): Get the current screen pointers */
void dgray_getscreen(uint32_t **light, uint32_t **dark)
{
	if(light) *light = vrams[0];
	if(dark) *dark = vrams[1];
}
