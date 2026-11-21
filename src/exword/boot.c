#include <gint/display.h>
#include <gint/exword.h>
#include <gint/mpu/mmu.h>

extern uint32_t mmu_exword_error[4];

static void hex(int y, char const *label, uint32_t value)
{
    char text[9];
    for(int i = 0; i < 8; i++)
        text[i] = "0123456789ABCDEF"[(value >> (28 - 4*i)) & 15];
    text[8] = 0;
    dtext(12, y, C_BLACK, label);
    dtext(150, y, C_BLACK, text);
}

static int keys(void)
{
    volatile uint16_t const *scan = (void *)0xa44b0000;
    uint16_t pressed = 0;
    for(int i = 0; i < 6; i++) pressed |= scan[i];
    return pressed;
}

void exword_boot_error(char const *stage)
{
    dclear(C_WHITE);
    dtext(12, 12, C_RED, "gint startup failed");
    dtext(12, 36, C_BLACK, stage);
    hex(64, "Virtual page", mmu_exword_error[0]);
    hex(84, mmu_exword_error[0] == 0x70000000 && !mmu_exword_error[3]
        ? "Image bytes" : "Expected physical", mmu_exword_error[1]);
    hex(104, "Actual physical", mmu_exword_error[2]);
    hex(124, "Page size", mmu_exword_error[3]);
    hex(144, "MMUCR", SH7305_MMU.MMUCR.lword);
    dtext(12, 180, C_BLACK, "Press any key to return to the menu.");
    dupdate();
    while(keys()) {}
    while(!keys()) {}
    while(keys()) {}
}
