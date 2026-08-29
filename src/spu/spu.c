#include <gint/mpu/spu.h>
#include <gint/mpu/cpg.h>
#include <gint/mpu/power.h>
#include <gint/mpu/fsi.h>
#include <gint/mpu/pfc.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/clock.h>
#include <gint/intc.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#define SPU   SH7305_SPU
#define DSP0  SH7305_DSP0
#define DSP1  SH7305_DSP1
#define CPG   SH7305_CPG
#define POWER SH7305_POWER
#define FSI   SH7305_FSI
#define PFC   SH7305_PFC

static void inth_dsp(int num)
{
/* C_RED comes from <gint/display-cg.h>, which GINT_RENDER_RGB selects; the
   fx-9860G-in-g3a build has HW_FX=0 but renders in mono. */
#if GINT_RENDER_MONO
	int fg = C_BLACK, bg = C_WHITE;
#else
	int fg = C_WHITE, bg = C_RED;
#endif
	dclear(bg);
	dprint(1, 1, fg, "DSP interrupt for DSP%d!", num);
	dupdate();
	getkey();
}

void spu_configure(void)
{
	// PFC.PSELA = PFC.PSELA & 0xf0ff | 0x100;
	PFC.PSELA = PFC.PSELA & 0xc0ff;

	intc_handler_function(0xcc0, GINT_CALL(inth_dsp, 0));
	intc_handler_function(0xce0, GINT_CALL(inth_dsp, 1));

	/* Log SPU interrupts from DSP0, DSP1, and their DMA */
	intc_priority(INTC_SPU_DSP0, 15);
	intc_priority(INTC_SPU_DSP1, 15);

	// SPU_PowerOnClocksAndModule

#if 0
	/* Stop both the SPU and FSI clocks */
	CPG.FSICLKCR.lword = 0x00000103;
	CPG.SPUCLKCR.lword = 0x00000100;
	/* Enable the FSI clock, then the SPU/SPURAM clock */
	CPG.FSICLKCR.CLKSTP = 0;
	CPG.SPUCLKCR.CLKSTP = 0;
	// TODO: CASIO driver sets SPUCLKCR to 3, not 0, when initializing?
#endif
	CPG.FSICLKCR.CLKSTP = 0;
	CPG.SPUCLKCR.lword = 0x00000003f;

	/* Power the clocks through MSTPCR2 */
	POWER.MSTPCR2.FSI_SPU = 0;

	// FSI_Initialize

	FSI.CLK_RST = 1;
	sleep_us_spin(10000);
	FSI.SOFT_RST = 0x11;
	sleep_us_spin(10000);
	FSI.SOFT_RST = 0x110;
	sleep_us_spin(10000);
	FSI.SOFT_RST = 0x111;
	sleep_us_spin(10000);

	FSI.A_DO_FMT = 0x20;
	FSI.A_DI_FMT = 0x20;
	FSI.A_CKG1 = 0x1111;
	FSI.CLK_RST = 1;
	sleep_us_spin(10000);

	// SPU_SoftwareReset

	/* Reset the SPU */
	SPU.SPUSRST.RST = 0;
	sleep_us_spin(10000);
	SPU.SPUSRST.RST = 1;

	/* Initially give all P memory and X memory to DSP0 */
	SPU.PBANKC0 = 0x1f;
	SPU.PBANKC1 = 0x00;
	SPU.XBANKC0 = 0x7f;
	SPU.XBANKC1 = 0x00;

	// Rest of SPU_Initialize

	/* Perform full DSP resets */
	DSP0.DSPCORERST = 1;
	DSP1.DSPCORERST = 1;
	DSP0.DSPRST = 1;
	DSP1.DSPRST = 1;
	sleep_us_spin(10000);
	DSP0.DSPRST = 0;
	DSP1.DSPRST = 0;
	sleep_us_spin(10000);
}

int spu_zero(void)
{
	return 0;
}

//---
// State and driver metadata
//---

static void hsave(spu_state_t *s)
{
	s->PBANKC0 = SPU.PBANKC0;
	s->PBANKC1 = SPU.PBANKC1;
	s->XBANKC0 = SPU.XBANKC0;
	s->XBANKC1 = SPU.XBANKC1;
}

static void hrestore(spu_state_t const *s)
{
	SPU.PBANKC0 = s->PBANKC0;
	SPU.PBANKC1 = s->PBANKC1;
	SPU.XBANKC0 = s->XBANKC0;
	SPU.XBANKC1 = s->XBANKC1;
}

gint_driver_t drv_spu = {
	.name         = "SPU",
	.configure    = NULL,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(spu_state_t),
};
GINT_DECLARE_DRIVER(16, drv_spu);
