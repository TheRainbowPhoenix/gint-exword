#include <gint/cpu.h>
#include <gint/drivers.h>
#include <gint/hardware.h>
#include <gint/kmalloc.h>
#include <gint/mmu.h>
#include <gint/rtc.h>
#include <gint/timer.h>
#include <stdlib.h>

static int initialized = 7;
static int constructed;
static volatile int ticks;

GCONSTRUCTOR static void construct(void)
{
    constructed = initialized;
}

static int tick(void)
{
    ticks++;
    return TIMER_STOP;
}

int main(void)
{
    uint32_t page_size;
    uint32_t physical = mmu_translate(0x74000000, &page_size);
    if(physical == (uint32_t)-1 || !page_size || !mmu_uram() ||
        mmu_uram_size() != 32768 || !mmu_is_rom((void *)0x70000080) ||
        !utlb_addr(0) || !utlb_data(0) || !itlb_addr(0) || !itlb_data(0) ||
        gint[HWCALC] != HWCALC_EXWORD || constructed != 7 ||
        !gint_driver_flags || !cpu_getSR().DSP || cpu_getVBR() != (uint32_t)mmu_uram())
        return -1;
    kmalloc_arena_t *arena = kmalloc_get_arena("_uram");
    void *p = malloc(64);
    if(!arena || !p || p < arena->start || p >= arena->end) return -1;
    void *q = realloc(p, 128);
    if(!q) { free(p); return -1; }
    free(q);
    int timer = timer_configure(TIMER_TMU, 1000, GINT_CALL(tick));
    if(timer < 0) return -1;
    uint32_t start = rtc_ticks();
    timer_start(timer);
    while(!ticks && ((rtc_ticks() + 11059200u - start) % 11059200u) < 256) {}
    timer_stop(timer);
    return ticks ? 0 : -1;
}
