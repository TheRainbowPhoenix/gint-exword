//---
//	gint:mmu:mmu - MMU driver definition and context management
//---

#include <gint/mmu.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/hardware.h>
#if GINT_HW_EX
#include <gint/cpu.h>
static uint32_t exword_rom, exword_ram, exword_boundary;
#endif

//---
//	Unified interface
//---

/* mmu_translate(): Get the physical address for a virtual page */
uint32_t mmu_translate(uint32_t page, uint32_t *size)
{
	return isSH3() ? tlb_translate(page,size) : utlb_translate(page,size);
}

/* mmu_uram(): Get pointer to physical start of user RAM */
void *mmu_uram(void)
{
	#if GINT_HW_EX
	return (void *)(exword_ram | 0x80000000);
	#else
	/* Use P1 access */
	return (void *)(mmu_translate(0x08100000, NULL) | 0x80000000);
	#endif
}

/* mmu_uram_size(): Get size of user RAM area */
uint32_t mmu_uram_size(void)
{
	#if GINT_HW_EX
	return exword_ram ? 0x8000 : 0;
	#else
	uint32_t size = 0;
	uint32_t pagesize;

	uint32_t base = GINT_HW_EX ? 0x74000000 : 0x08100000;
	while(mmu_translate(base + size, &pagesize) != (uint32_t)-1)
	{
		size += pagesize;
	}

	return size;
	#endif
}

/* mmu_is_rom(): Determine if an address points to ROM */
bool mmu_is_rom(void const *ptr)
{
	uint32_t a = (uint32_t)ptr;

	if(a >= 0x80000000 && a < 0x88000000)
		return true;
	if(a >= 0xa0000000 && a < 0xa8000000)
		return true;
	#if GINT_HW_EX
	if(a >= 0x70000000 && a < 0x70020000)
		return true;
	#else
	if(a >= 0x00300000 && a < 0x00800000)
		return true;
	#endif

	return false;
}

//---
//	SH7705 TLB
//---

#if GINT_HW_FX

/* tlb_addr() - get the P4 address of a TLB address entry */
GINLINE const tlb_addr_t *tlb_addr(uint way, uint E)
{
	uint32_t addr = 0xf2000000 | (E << 12) | (way << 8);
	return (void *)addr;
}

/* tlb_data() - get the P4 address of a TLB data entry */
GINLINE const tlb_data_t *tlb_data(uint way, uint E)
{
	uint32_t addr = 0xf3000000 | (E << 12) | (way << 8);
	return (void *)addr;
}

/* tlb_mapped_memory() - count amount of mapped memory */
void tlb_mapped_memory(uint32_t *p_rom, uint32_t *p_ram)
{
	uint32_t rom = 0, ram = 0;

	for(int way = 0; way < 4; way++)
	for(int E = 0; E < 32; E++)
	{
		const tlb_addr_t *addr = tlb_addr(way, E);
		const tlb_data_t *data = tlb_data(way, E);
		if(!addr->V || !data->V) continue;

		int size = data->SZ ? 4096 : 1024;
		uint32_t src;

		if(data->SZ) src = (((addr->VPN >> 2) | E) << 12);
		else src = (addr->VPN | (E << 2)) << 10;

		if(src >= 0x00300000 && src < 0x00380000) rom += size;
		if(src >= 0x08100000 && src < 0x08180000) ram += size;
	}

	if(p_rom) *p_rom = rom;
	if(p_ram) *p_ram = ram;

	gint[HWURAM] = ram;
}

/* tlb_translate(): Get the physical address for a virtual page */
uint32_t tlb_translate(uint32_t page, uint32_t *size)
{
	for(int way = 0; way < 4; way++)
	for(int E = 0; E < 32; E++)
	{
		const tlb_addr_t *addr = tlb_addr(way, E);
		const tlb_data_t *data = tlb_data(way, E);
		if(!addr->V || !data->V) continue;

		uint32_t src;
		if(data->SZ) src = (((addr->VPN >> 2) | E) << 12);
		else src = (addr->VPN | (E << 2)) << 10;

		if(src == page)
		{
			if(size) *size = (data->SZ ? 4096 : 1024);
			return data->PPN << 10;
		}
	}
	return -1;
}
#endif

//---
//	SH7305 Unified TLB
//---

GINLINE const utlb_addr_t *utlb_addr(uint E)
{
	uint32_t addr = 0xf6000000 | ((E & 0x3f) << 8);
	return (void *)addr;
}

GINLINE const utlb_data_t *utlb_data(uint E)
{
	uint32_t addr = 0xf7000000 | ((E & 0x3f) << 8);
	return (void *)addr;
}

void utlb_mapped_memory(uint32_t *p_rom, uint32_t *p_ram)
{
	uint32_t rom = 0, ram = 0;

	for(int E = 0; E < 64; E++)
	{
		const volatile utlb_addr_t *addr = utlb_addr(E);
		const volatile utlb_data_t *data = utlb_data(E);
		if(!addr->V || !data->V) continue;

		/* Magic formula to get the size without using an array since
		   this code is used even before global data is initialized */
		int sz = ((data->SZ1 << 1) | data->SZ2) << 3;
		int size = 1 << ((0x14100c0a >> sz) & 0xff);

		uint32_t src = addr->VPN << 10;
		#if GINT_HW_EX
		if(src >= 0x70000000 && src < 0x70020000) rom += size;
		if(src >= 0x74000000 && src < 0x74008000) ram += size;
		#else
		if(src >= 0x00300000 && src < 0x00700000) rom += size;
		if(src >= 0x08100000 && src < 0x08180000) ram += size;
		#endif
	}

	if(p_rom) *p_rom = rom;
	if(p_ram) *p_ram = ram;

	gint[HWURAM] = ram;
}

uint32_t utlb_translate(uint32_t page, uint32_t *size)
{
	for(int E = 0; E < 64; E++)
	{
		const volatile utlb_addr_t *addr = utlb_addr(E);
		const volatile utlb_data_t *data = utlb_data(E);
		if(!addr->V || !data->V) continue;

		if((uint32_t)addr->VPN << 10 == page)
		{
			/* Same magic formula as utlb_mapped_memory() */
			int sz = ((data->SZ1 << 1) | data->SZ2) << 3;
			if(size) *size = 1 << ((0x14100c0a >> sz) & 0xff);

			return data->PPN << 10;
		}
	}
	return -1;
}

itlb_addr_t const *itlb_addr(uint E)
{
	uint32_t addr = 0xf2000000 | ((E & 3) << 8);
	return (void *)addr;
}

itlb_data_t const *itlb_data(uint E)
{
	uint32_t addr = 0xf3000000 | ((E & 3) << 8);
	return (void *)addr;
}

static void configure(void)
{
	/* Make writes to the control register area synchronous; this is needed
	   for the SPU to operate properly */
	if(isSH4()) SH7305_MMU.PASCR.UBC = 1;
}

#if GINT_HW_EX
/* Runs through P2 while changing the TLB. */
static __attribute__((noinline)) void exword_pin(uint32_t ram, uint32_t control)
{
	volatile uint32_t *addresses = (void *)0xf6000000;
	volatile uint32_t *entries = (void *)0xf7000000;
	SH7305_MMU.MMUCR.lword = (SH7305_MMU.MMUCR.lword & ~0x00fcfc00u)
		| (control & 0x00fcfc00u);
	__asm__ volatile("icbi @%0" :: "r"(0xa0000000u) : "memory");
	if(!ram) return;
	uint32_t boundary = (control >> 18) & 63;
	for(int i = 0; i < 64; i++) {
		uint32_t a = addresses[i * 64];
		uint32_t page = a & 0xfffffc00;
		if(page >= 0x74000000 && page < 0x74008000)
			addresses[i * 64] = a & ~0x100u;
	}
	for(uint32_t i = 0; i < 8; i++) {
		uint32_t slot = (boundary + i) * 64;
		addresses[slot] = 0;
		entries[slot] = (ram + i * 4096) | 0x17c;
		addresses[slot] = (0x74000000 + i * 4096) | 0x300;
	}
	__asm__ volatile("icbi @%0" :: "r"(0xa0000000u) : "memory");
}

static void exword_mmu(uint32_t ram, uint32_t control)
{
	void (*pin)(uint32_t, uint32_t) = (void *)
		(0xa0000000u | (exword_rom + (uint32_t)exword_pin - 0x70000000u));
	pin(ram, control);
}

static void bind(void)
{
	exword_mmu(exword_ram, exword_boundary << 18);
}
#endif

//---
// State and driver metadata
//---

static void hsave(mmu_state_t *s)
{
	if(isSH3()) return;
	s->PASCR = SH7305_MMU.PASCR.lword;
	s->IRMCR = SH7305_MMU.IRMCR.lword;
	#if GINT_HW_EX
	s->MMUCR = SH7305_MMU.MMUCR.lword;
	#endif
}

static void hrestore(mmu_state_t const *s)
{
	if(isSH3()) return;
	SH7305_MMU.PASCR.lword = s->PASCR;
	SH7305_MMU.IRMCR.lword = s->IRMCR;
	#if GINT_HW_EX
	exword_mmu(0, s->MMUCR);
	#endif
}

gint_driver_t drv_mmu = {
	.name         = "MMU",
	.configure    = configure,
	#if GINT_HW_EX
	.bind         = bind,
	#endif
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(mmu_state_t),
};
GINT_DECLARE_DRIVER(02, drv_mmu);

#if GINT_HW_EX
uint32_t mmu_exword_error[4];

uint32_t mmu_exword_rom(void)
{
	return exword_rom;
}

static uint32_t exword_image_base(uint32_t length)
{
	for(int i = 0; i < 64; i++) {
		utlb_addr_t a = *(volatile utlb_addr_t const *)utlb_addr(i);
		utlb_data_t d = *(volatile utlb_data_t const *)utlb_data(i);
		uint32_t page = a.VPN << 10;
		if(a.V && d.V && !d.SZ1 && d.SZ2 && page >= 0x70000000 &&
			page - 0x70000000 < length)
			return (d.PPN << 10) - (page - 0x70000000);
	}
	for(int i = 0; i < 4; i++) {
		itlb_addr_t a = *(volatile itlb_addr_t const *)itlb_addr(i);
		itlb_data_t d = *(volatile itlb_data_t const *)itlb_data(i);
		uint32_t page = a.VPN << 10;
		if(a.V && d.V && !d.SZ1 && d.SZ0 && page >= 0x70000000 &&
			page - 0x70000000 < length)
			return (d.PPN << 10) - (page - 0x70000000);
	}
	return (uint32_t)-1;
}

int mmu_exword_prepare(uint32_t uram)
{
	extern uint32_t srom;
	uint32_t length = (uint32_t)&srom;
	mmu_exword_error[0] = 0x70000000;
	mmu_exword_error[1] = length;
	mmu_exword_error[2] = (uint32_t)-1;
	mmu_exword_error[3] = 0;
	if(length <= 0x80 || length > 0x20000) return 0;
	if(uram && ((uram & 0xe0000fff) != 0x80000000)) return 0;
	if(uram) exword_ram = uram & 0x1fffffff;
	if(!exword_ram) return 0;

	cpu_atomic_start();
	uint32_t control = SH7305_MMU.MMUCR.lword;
	uint32_t available = (control >> 18) & 63;
	if(!available) available = 64;
	exword_rom = exword_image_base(length);
	int valid = (control & 0x81) == 1 && available > 8 &&
		exword_rom != (uint32_t)-1 && !(exword_rom & 4095);
	for(int i = 0; valid && i < 64; i++) {
		utlb_addr_t a = *(volatile utlb_addr_t const *)utlb_addr(i);
		utlb_data_t d = *(volatile utlb_data_t const *)utlb_data(i);
		uint32_t page = a.VPN << 10;
		if(!a.V || !d.V || page < 0x74000000 || page >= 0x74008000)
			continue;
		mmu_exword_error[0] = page;
		mmu_exword_error[1] = exword_ram + page - 0x74000000;
		mmu_exword_error[2] = d.PPN << 10;
		int shift = ((d.SZ1 << 1) | d.SZ2) << 3;
		mmu_exword_error[3] = 1u << ((0x14100c0au >> shift) & 255);
		valid = i < (int)available && mmu_exword_error[3] == 4096 &&
			mmu_exword_error[1] == mmu_exword_error[2];
	}
	if(valid) {
		exword_boundary = available - 8;
		gint[HWURAM] = 0x8000;
	}
	cpu_atomic_end();
	return valid;
}
#endif
