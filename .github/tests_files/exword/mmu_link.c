#include <gint/hardware.h>
#include <gint/mmu.h>

int main(void)
{
    uint32_t page_size, rom_size, ram_size;
    hw_detect();
    utlb_mapped_memory(&rom_size, &ram_size);
    uint32_t physical = mmu_translate(0x74000000, &page_size);
    return physical == (uint32_t)-1 || !page_size || !mmu_uram() ||
        !mmu_uram_size() || !mmu_is_rom((void *)0x70000080) ||
        !utlb_addr(0) || !utlb_data(0) || !itlb_addr(0) || !itlb_data(0) ||
        gint[HWCALC] != HWCALC_EXWORD;
}
