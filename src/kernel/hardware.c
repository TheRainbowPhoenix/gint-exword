//---
//	gint:core:hardware - Platform information and hardware detection
//---

#include <gint/hardware.h>
#include <gint/mmu.h>
#include <gint/defs/attributes.h>
#include <gint/defs/types.h>
#include <gint/defs/util.h>
#include <gint/mpu/pfc.h>
#include <gint/config.h>

#include <string.h>
#include "kernel.h"

/* Holds information about the current platform */
GBSS uint32_t gint[HW_KEYS];

/* Processor Version Register */
#define PVR (*((volatile uint32_t *)0xff000030))
/* Product Register */
#define PRR (*((volatile uint32_t *)0xff000044))

#if GINT_HW_FX

/* mpu_detect() - detect the underlying MPU
   Many thanks to Simon Lothar for relevant documentation.

   Processor Version Register (PVR) and Product Version Register (PRR) provide
   info for SH-4-based MPUS; SH-3 based boards are detected and distinguished
   by testing writable bits in the Port L Control Register (PLCR).

   Returns the detected MPU type, falling back on mpu_unknown */
static int mpu_detect(void)
{
	#define PLCR SH7705_PFC.PLCR

	/* Detect SH-3-based MPUs by testing writable bits in PLCR */
	uint16_t old = PLCR;
	PLCR = 0xffff;
	uint16_t tested = PLCR;
	PLCR = old;

	if(tested == 0x00ff) return HWMPU_SH7337;
	if(tested == 0x0fff) return HWMPU_SH7355;

	/* Check that we're dealing with an SH-4-based MPU */
	if((PVR & 0xffffff00) != 0x10300b00) return HWMPU_UNKNOWN;

	/* Tell SH-4 MPUs by testing the product version register */

	uint32_t ver = PRR & 0xfffffff0;
	if(ver == 0x00002c00) return HWMPU_SH7305;
	if(ver == 0x00002200) return HWMPU_SH7724;

	return HWMPU_UNKNOWN;
	#undef PLCR
}

/* hw_detect(): Basic hardware detection */
void hw_detect(void)
{
	gint[HWMPU] = mpu_detect();

	if(isSH4())
	{
		gint[HWCPUVR] = PVR;
		gint[HWCPUPR] = PRR;
	}

	gint[HWCALC] = HWCALC_FX9860G_SH4;
	if(gint[HWMPU] == HWMPU_SH7337 || gint[HWMPU] == HWMPU_SH7355)
	{
		gint[HWCALC] = (SH7705_PFC.PEDR & 0x08) ? HWCALC_FX9860G_SH3 :
				HWCALC_FX9860G_SLIM;
	}

	/* Tell Graph 35+E II from OS version (this is accurate unless someone
	   tweaks an OS file enough to de-correlate the version of the OS and
	   the version of the display and storage memory drivers, which, let's
	   be real, is enough for now.
	   TODO: Try to detect Graph 35+E II from amount of ROM in BSC? */
	char *version = (void *)0x80010020;
	if(version[1] == '3')
	{
		gint[HWCALC] = HWCALC_G35PE2;
		gint[HWFS] = HWFS_FUGUE;
	}
	else
	{
		gint[HWFS] = HWFS_CASIOWIN;
	}

	/* Detect RAM by checking if a804'0000 is the same as a800'0000. */

	volatile uint8_t *R4 = (void *)0xa8040000;
	volatile uint8_t *R0 = (void *)0xa8000000;

	/* Make backups */
	uint8_t b0 = *R0;
	uint8_t b4 = *R4;

	/* Check if setting a different value in *R4 affects *R0. If not, then
	   we have extended RAM. */
	*R4 = ~b0;
	int ext = (*R0 == b0);

	/* Restore backups */
	*R0 = b0;
	*R4 = b4;

	gint[HWRAM]  = ext ? (512 << 10) : (256 << 10);
	/* Traditionally 4 MiB, Graph 35+E II has 8 MiB */
	gint[HWROM]  = (gint[HWCALC] == HWCALC_G35PE2) ? (8 << 20) : (4 << 20);

	/* Mapped memory */
	if(isSH3()) tlb_mapped_memory(NULL, NULL);
	else utlb_mapped_memory(NULL, NULL);
}

#elif GINT_HW_CG

/* hw_detect(): Basic hardware detection */
void hw_detect(void)
{
	gint[HWMPU] = HWMPU_SH7305;
	gint[HWCPUVR] = PVR;
	gint[HWCPUPR] = PRR;

	uint32_t stack;
	__asm__("mov r15, %0" : "=r"(stack));
	char const *version = (void *)0x80020020;
	char const *osdate = (void *)0x80b5ffe0;

	/* Tell Prizms apart from fx-CG 50 by checking the stack address*/
	if(stack < 0x8c000000) {
		gint[HWCALC] = HWCALC_PRIZM;
	}
	/* Tell Math+/fx-CG 100 apart from CG-50 by checking OS version + date.
	   CG-50 OS versions use OS 3. Math+, for some reason, rewinds back to OS 1
	   and got updated to OS 2 in late 2024. We decide that we are on a CG-50 OS
	   if the version is 3 and the date is 201x-2024. */
	else {
		/* All known CG-50 versions have date at this address due to the footer
		   location. WARNING: not a future-proof address! */
		char d0 = osdate[0], d1 = osdate[1], d2 = osdate[2], d3 = osdate[3];
		bool cg50 = version[1] == '3' && d0 == '2' && d1 == '0' &&
						(d2 == '1' || (d2 == '2' && d3 <= '4'));
		gint[HWCALC] = cg50 ? HWCALC_FXCG50 : HWCALC_FXCG100;
	}

	gint[HWFS] = HWFS_FUGUE;

	/* Tell the fx-CG emulator apart using the product ID */
	uint8_t *productID = (void *)0x8001ffd0;
	if(!memcmp(productID, "\xff\xff\xff\xff\xff\xff\xff\xff", 8))
		gint[HWCALC] = HWCALC_FXCG_MANAGER;

	/* Basic memory information */
	gint[HWRAM] = (gint[HWCALC] == HWCALC_PRIZM) ? (2 << 20) : (8 << 20);
	gint[HWROM] = (32 << 20);

	/* Mapped memory */
	utlb_mapped_memory(NULL, NULL);
}

#elif GINT_HW_CP

/* hw_detect(): Basic hardware detection */
void hw_detect(void)
{
	gint[HWMPU] = HWMPU_SH7305;
	gint[HWCPUVR] = PVR;
	gint[HWCPUPR] = PRR;
	gint[HWCALC] = HWCALC_FXCP400;
	// TODO: What filesystem implementation on the fx-CP 400?
	gint[HWFS] = HWFS_NONE;
	gint[HWRAM] = 16 << 20;
	// TOOD: How much ROM on the fx-CP 400?
	gint[HWROM] = 0;

	/* There is no userspace so not MMU being used */
	gint[HWURAM] = 0;
}

#else
#error unknown hardware type for hw_detect
#endif
