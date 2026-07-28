// Renesas R61523 driver-

#include <gint/defs/types.h>
#include <gint/defs/util.h>
#include <gint/hardware.h>
#include <gint/drivers.h>
#include <gint/dma.h>
#include <gint/drivers/r61523.h>
#include <gint/video.h>
#include <gint/image.h>
#include <gint/config.h>

#if GINT_HW_CP

/* Registers */
#define REG_HRANGE                0x2a
#define REG_VRANGE                0x2b
#define REG_DATA                  0x2c
#define REG_DEVICE_CODE_READ      0xbf
#define REG_DEVICE_CODE_VARIANT   0xda

/* Interface with the controller */
static volatile uint16_t *DISPLAY = (void *)0xb4000000;
/* Bit 4 of Port R controls the RS bit of the display driver */
static volatile uint8_t *PRDR = (void *)0xa405013c;

/* Screen variant information
 * This number is 0x00 for the old R61523 that everyone knows, and 0x16 or
 * 0x52 for the newer ones found in recent FXCP400 devices. */
static uint16_t r61523_variant = 0;

/* Select a register */
GINLINE static void select(uint16_t reg)
{
    /* Clear RS and write the register number */
    *PRDR &= ~0x10;
    synco();
    *DISPLAY = reg;
    synco();
    /* Set RS=1 to allow consecutive reads/writes after a select() */
    *PRDR |= 0x10;
    synco();
}

GINLINE static void write(uint16_t data)
{
    *DISPLAY = data;
}

GINLINE static uint16_t read(void)
{
    return *DISPLAY;
}

static void read_Nu16(uint16_t *array, int N)
{
    for(int i = 0; i < N; i++)
        array[i] = *DISPLAY;
}

GINLINE uint16_t r61523_get(int ID)
{
    select(ID);
    return *DISPLAY;
}

GINLINE void r61523_set(int ID, uint16_t value)
{
    select(ID);
    *DISPLAY = value;
}

//---
// Generic functions
//---

/* r61523_identify() - identify screen hardware information
 *
 * notes
 *   Since recent versions of the FXCP400, new screens are used that break
 *   the current driver. These screens are particular since they have the
 *   same manufacturer / device code, but they use an undocumented register
 *   (0xda) to determine the variant.
 *
 *   The register 0xda should be read twice. Casio ignores the first reading
 *   and only keep track of the second. So, let's do the same here. */
void r61523_identify(
    uint32_t *manufacturerCode,
    uint16_t *deviceCode,
    uint16_t *variant
) {
    uint16_t packets[5];

    if (manufacturerCode != NULL || deviceCode != NULL)
    {
        select(REG_DEVICE_CODE_READ);
        read_Nu16(packets, 5);
        if(manufacturerCode)
            *manufacturerCode = (packets[1] << 16) | packets[2];
        if(deviceCode)
            *deviceCode = (packets[3] << 16) | packets[4];
    }
    if (variant)
    {
        select(REG_DEVICE_CODE_VARIANT);
        read();
        *variant = read();
    }
}

//---
// Window management
//---

void r61523_win_set(int x1, int x2, int y1, int y2)
{
    if (r61523_variant != 0x16 || r61523_variant != 0x52)
    {
        /* R61523 has a 360x640 area; the CP-400 uses the top-right corner
         * for its 320x528 display, so skip over the first 40 columns */
        x1 += 40;
        x2 += 40;
    }

    uint16_t volatile *DISPLAY = (void *)0xb4000000;

    select(REG_HRANGE);

    /* Upper half has 2 bits (total 10 bits = 1024) */
    *DISPLAY = (x1 >> 8) & 3;
    synco();
    *DISPLAY = x1 & 0xff;

    *DISPLAY = (x2 >> 8) & 3;
    synco();
    *DISPLAY = x2 & 0xff;
    synco();

    select(REG_VRANGE);

    *DISPLAY = (y1 >> 8) & 3;
    synco();
    *DISPLAY = y1 & 0xff;
    synco();

    *DISPLAY = (y2 >> 8) & 3;
    synco();
    *DISPLAY = y2 & 0xff;
    synco();
}

void r61523_start_frame(int xmin, int xmax, int ymin, int ymax) {
    r61523_win_set(xmin, xmax, ymin, ymax);
    select(REG_DATA);
}

void r61523_display(uint16_t *vram, int start, int height, int method)
{
    /* Wait for any transfer to finish before using the screen, otherwise
       the DMA might write data *while* we're sending commands! */
    dma_transfer_wait(0);

    r61523_start_frame(0, 319, start, start + height - 1);

    if(method == R61523_CPU)
    {
        for(int i = 0; i < 320 * height; i++)
            write(vram[i + 320 * start]);
        return;
    }

    void *src = (void *)vram + start * 320*2;
    void *dst = (void *)DISPLAY;

    /* Luckly the row count is a multiple of 32.. */
    int blocks = height * 20;

    if(method == R61523_DMA) {
        dma_transfer_async(0, DMA_32B, blocks, src, DMA_INC, dst,
            DMA_FIXED, GINT_CALL_NULL);
    }
    else {
        /* Transfer atomically */
        dma_transfer_atomic(0, DMA_32B, blocks, src, DMA_INC, dst,
            DMA_FIXED);
    }
}

void r61523_display_rect(
    uint16_t *vram, int xmin, int xmax, int ymin, int ymax)
{
    dma_transfer_wait(0);
    r61523_start_frame(xmin, xmax, ymin, ymax);

    vram += 320 * ymin + xmin;

    for(int y = 0; y < ymax - ymin + 1; y++) {
        for(int x = 0; x < xmax - xmin + 1; x++)
            write(vram[x]);
        vram += 320;
    }
}

void r61523_set_pixel(int x, int y, int color)
{
    if((unsigned)x >= 320 || (unsigned)y >= 528)
        return;

    dma_transfer_wait(0);
    r61523_start_frame(x, x, y, y);
    write(color);
}

static bool r61523_update(int x, int y, image_t const *fb, int flags)
{
    // TODO: r61523_update: Handle the mono cases
    if(fb->format != IMAGE_RGB565)
        return false;

    uint w = fb->width;
    uint h = fb->height;

    dma_transfer_wait(0);
    r61523_start_frame(x, x+w-1, y, y+h-1);

    /* DMA if enabled */
    bool dma_possible = (!x && w == 320 && fb->stride == 320*2);
    if((flags & VIDEO_UPDATE_ENABLE_DMA) && dma_possible) {
        void *src = fb->data;
        void *dst = (void *)DISPLAY;
        int blocks = h * 20;

        if(flags & VIDEO_UPDATE_ATOMIC)
            dma_transfer_atomic(0, DMA_32B, blocks, src, DMA_INC,
                dst, DMA_FIXED);
        else
            dma_transfer_async(0, DMA_32B, blocks, src, DMA_INC,
                dst, DMA_FIXED, GINT_CALL_NULL);
        return true;
    }

    uint16_t *pixels = fb->data;

    for(int y = 0; y < fb->height; y++) {
        for(int x = 0; x < fb->width; x++)
            write(pixels[x]);
        pixels = (void *)pixels + fb->stride;
    }

    return true;
}

//---
// Driver metadata
//---

/* constructor() - determine which variant of screen we have */
static void constructor(void)
{
    r61523_identify(NULL, NULL, &r61523_variant);
}

/* As far as I can tell there's no way to read the current window from the
   controller so this driver is completely stateless for now. */
gint_driver_t drv_r61523 = {
    .name        = "R61523",
    .constructor = constructor,
};
GINT_DECLARE_DRIVER(26, drv_r61523);

//---
// Video driver interface
//---

static video_mode_t r61523_modes[] = {
    /* Standard full-screen full-color mode */
    { 320, 528, IMAGE_RGB565, -1 },
    { 0 }
};

video_interface_t r61523_video = {
    .driver          = &drv_r61523,
    .modes           = r61523_modes,
    .mode_get        = NULL, // TODO
    .mode_set        = NULL, // TODO
    .brightness_min  = 0, // TODO
    .brightness_max  = 0, // TODO
    .brightness_set  = NULL,
    .update          = r61523_update,
};

#endif /* GINT_HW_CP */
