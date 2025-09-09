#include <gint/display.h>
#include <gint/dma.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void dclear(uint16_t color)
{
	// TODO: CP: DMA support for dclear()
#if GINT_HW_CP
	uint32_t color4 = (color << 16) | color;
	for(int i = 0; i < DWIDTH * DHEIGHT / 2; i++)
		((uint32_t *)gint_vram)[i] = color4;
#else
	bool full_width = (dwindow.left == 0 && dwindow.right == DWIDTH);
	bool dma_aligned = !(dwindow.top & 3) && !(dwindow.bottom & 3);

	if(full_width && dma_aligned) {
		uint16_t *vram = gint_vram + DWIDTH * dwindow.top;
		int size_bytes = DWIDTH * (dwindow.bottom - dwindow.top) * 2;
		dma_memset(vram, (color << 16) | color, size_bytes);
	}
	else {
		drect(dwindow.left, dwindow.top, dwindow.right - 1,
			dwindow.bottom - 1, color);
	}
#endif
}

#endif
