#include "test.h"
#include <gint/display.h>
#include <gint/image.h>
#include <gint/cpu.h>
#include <gint/mpu/cpg.h>
#include <gint/mpu/power.h>
#include <gint/mpu/tmu.h>

extern image_t test_rgb565, test_rgb565a;
extern image_t test_p4_rgb565, test_p4_rgb565a;
extern image_t test_p8_rgb565, test_p8_rgb565a;
extern font_t test_font;

static int channel = -1;
static uint32_t saved_cor, saved_cnt, previous, ticks_per_second;
static uint16_t saved_cr;
static int saved_stop;
static uint64_t elapsed;
static unsigned displayed = (unsigned)-1;
static unsigned char converted_pixels[40 * 40 * 2] __attribute__((aligned(4)));

static int clock_start(void)
{
    if(channel >= 0) return 1;
    uint32_t base = 32768;
    if(SH7305_CPG.PLLCR.FLLE) {
        unsigned fll = SH7305_CPG.FLLFRQ.FLF;
        if(SH7305_CPG.FLLFRQ.SELXM == 1) fll >>= 1;
        base *= fll;
    }
    if(SH7305_CPG.PLLCR.PLLE) base *= SH7305_CPG.FRQCR.STC + 1;
    ticks_per_second = (base >> (SH7305_CPG.FRQCR.P1FC + 1)) / 256;
    if(!ticks_per_second) return 0;
    cpu_atomic_start();
    saved_stop = SH7305_POWER.MSTPCR0.TMU0;
    SH7305_POWER.MSTPCR0.TMU0 = 0;
    (void)SH7305_POWER.MSTPCR0.lword;
    for(int i = 2; i >= 0; i--) {
        if(!(SH7305_TMU.TSTR & (1 << i)) && !SH7305_TMU.TMU[i].TCR.UNIE) {
            channel = i;
            break;
        }
    }
    if(channel >= 0) {
        tmu_t *t = &SH7305_TMU.TMU[channel];
        saved_cor = t->TCOR;
        saved_cnt = t->TCNT;
        saved_cr = t->TCR.word;
        t->TCR.word = 3;
        t->TCOR = t->TCNT = 0xffffffff;
        previous = t->TCNT;
        SH7305_TMU.TSTR |= 1 << channel;
        elapsed = 0;
        displayed = (unsigned)-1;
    }
    else SH7305_POWER.MSTPCR0.TMU0 = saved_stop;
    cpu_atomic_end();
    return channel >= 0;
}

void test_clock_stop(void)
{
    if(channel < 0) return;
    cpu_atomic_start();
    SH7305_TMU.TSTR &= ~(1 << channel);
    tmu_t *t = &SH7305_TMU.TMU[channel];
    t->TCOR = saved_cor;
    t->TCNT = saved_cnt;
    t->TCR.word = saved_cr;
    if(!(SH7305_TMU.TSTR & 7)) SH7305_POWER.MSTPCR0.TMU0 = saved_stop;
    channel = -1;
    cpu_atomic_end();
}

void test_clock_update(void)
{
    if(channel < 0) return;
    uint32_t now = SH7305_TMU.TMU[channel].TCNT;
    elapsed += (uint32_t)(previous - now);
    previous = now;
    unsigned seconds = elapsed / ticks_per_second;
    if(seconds == displayed) return;
    displayed = seconds;
    unsigned minutes = seconds / 60;
    char text[] = "TMU 000:00";
    text[4] = '0' + minutes / 100 % 10;
    text[5] = '0' + minutes / 10 % 10;
    text[6] = '0' + minutes % 10;
    text[8] = '0' + seconds / 10 % 6;
    text[9] = '0' + seconds % 10;
    drect(390, 26, DWIDTH-1, 43, C_WHITE);
    dtext(392, 28, C_BLACK, text);
    dupdate_rect(390, 26, DWIDTH-1, 43);
}

void test_screen(void)
{
    dclear(C_WHITE);
    drect(0, 0, DWIDTH-1, 23, C_LIGHT);
    dtext(8, 7, C_BLACK, "DRAW");
    dtext(140, 7, C_BLACK, "TEST");
    dtext(420, 7, C_BLACK, "QUIT");
    dtext(8, 30, C_BLACK, "TOPTI DEFAULT AaZz 0123456789 !?");
    font_t const *old = dfont(&test_font);
    dtext(8, 49, C_BLUE, "FXCONV FONT 5x7 AaZz 0123456789");
    dfont(old);
    dtext_opt(8, 65, C_BLACK, 0xffe0, DTEXT_LEFT, DTEXT_TOP,
        "Text with background", -1);
    dtext_opt(520, 65, C_BLUE, C_NONE, DTEXT_RIGHT, DTEXT_TOP,
        "Right aligned", -1);
    image_t const *images[] = { &test_rgb565, &test_rgb565a,
        &test_p4_rgb565, &test_p4_rgb565a, &test_p8_rgb565, &test_p8_rgb565a };
    char const *labels[] = {"RGB", "RGB+A", "P4", "P4+A", "P8", "P8+A"};
    for(int i = 0; i < 6; i++) {
        int x = 12 + 85 * i;
        dtext(x, 91, C_BLACK, labels[i]);
        for(int y = 0; y < 40; y += 5)
            for(int z = 0; z < 40; z += 5)
                drect(x+z, 107+y, x+z+4, 111+y,
                    ((y+z)/5 & 1) ? C_LIGHT : 0xffe0);
        dimage(x, 107, images[i]);
    }
    image_t converted = {.format = IMAGE_RGB565, .width = 40, .height = 40,
        .stride = 80, .data = converted_pixels};
    image_copy(&test_p8_rgb565, &converted, true);
    dimage(12, 169, &converted);
    dtext(64, 180, C_BLACK, "P8 -> RGB conversion");
    drect_border(12, 227, 82, 278, C_RED, 2, C_BLACK);
    dcircle(122, 252, 25, C_BLUE, C_BLACK);
    dellipse(170, 230, 240, 278, C_GREEN, C_BLACK);
    dline(260, 230, 325, 278, C_BLUE);
    dline(325, 230, 260, 278, C_RED);
    int x[] = {350, 380, 410}, y[] = {278, 230, 278};
    for(int i = 0; i < 3; i++)
        dline(x[i], y[i], x[(i+1)%3], y[(i+1)%3], C_BLACK);
    dtext(8, 300, C_BLACK, "EXE resets clock  EXIT quits  touch DRAW returns");
    if(!clock_start()) dtext(390, 28, C_RED, "TMU unavailable");
    dupdate();
    test_clock_update();
}

