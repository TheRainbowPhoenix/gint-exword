//---
// gint:cpg:overclock - Clock speed control
//
// Most of the data in this file has been reused from Sentaro21's Ftune and
// Ptune utilities, which have long been the standard for overclocking CASIO
// calculators.
// See: http://pm.matrix.jp/ftune2e.html
//
// SlyVTT also contributed early testing on both the fx-CG 10/20 and fx-CG 50.
//---

#include <gint/clock.h>
#include <gint/gint.h>
#include <gint/hardware.h>
#include <gint/mpu/cpg.h>
#include <gint/mpu/bsc.h>
#include <gint/mpu/wdt.h>

#define CPG SH7305_CPG
#define BSC SH7305_BSC

#define CPGSH3 SH7705_CPG
#define BSCSH3 SH7705_BSC
#define WDTSH3 SH7705_WDT

//---
// Low-level clock speed access
//---

#define SDMR3_CL2 ((volatile uint8_t *)0xFEC15040)
#define SDMR3_CL3 ((volatile uint8_t *)0xFEC15060)


//---
// Predefined clock speeds
//---

// for SH4 based calculators
#define SH4_PLL_32x  0b011111
#define SH4_PLL_26x  0b011001
#define SH4_PLL_16x  0b001111
#define SH4_DIV_2    0
#define SH4_DIV_4    1
#define SH4_DIV_8    2
#define SH4_DIV_16   3
#define SH4_DIV_32   4

// for SH3 based calculators
#define SH3_PLL_1x 0 //0b0000	// x1
#define SH3_PLL_2x 1 //0b0001	// x2
#define SH3_PLL_3x 2 //0b0010	// x3
#define SH3_PLL_4x 3 //0b0011	// x4

#define SH3_DIV_1 0 //0b0000	// 1/1
#define SH3_DIV_2 1 //0b0001	// 1/2
#define SH3_DIV_3 2 //0b0010	// 1/3
#define SH3_DIV_4 3 //0b0011	// 1/4


void cpg_get_overclock_setting(struct cpg_overclock_setting *s)
{
    if(isSH3())
    {
      s->FLLFRQ = 0xFFFFFFF;    // not used for SH3 MPUs
      s->FRQCR = CPGSH3.FRQCR.word;

      s->CS0BCR = BSCSH3.CS0BCR.lword;
      s->CS0WCR = BSCSH3.CS0WCR.lword;
      s->CS2BCR = BSCSH3.CS2BCR.lword;
      s->CS2WCR = BSCSH3.CS2WCR.lword;
      s->CS3BCR = BSCSH3.CS3BCR.lword;
      s->CS3WCR = BSCSH3.CS3WCR.lword;
      s->CS5aBCR = BSCSH3.CS5ABCR.lword;
      s->CS5aWCR = BSCSH3.CS5AWCR.lword;
    }
    else if(isSH4())
    {
      s->FLLFRQ = CPG.FLLFRQ.lword;
      s->FRQCR = CPG.FRQCR.lword;

      s->CS0BCR = BSC.CS0BCR.lword;
      s->CS0WCR = BSC.CS0WCR.lword;
      s->CS2BCR = BSC.CS2BCR.lword;
      s->CS2WCR = BSC.CS2WCR.lword;
      s->CS3BCR = BSC.CS3BCR.lword;
      s->CS3WCR = BSC.CS3WCR.lword;
      s->CS5aBCR = BSC.CS5ABCR.lword;
      s->CS5aWCR = BSC.CS5AWCR.lword;
    }
    else return;
}

void cpg_set_overclock_setting(struct cpg_overclock_setting const *s)
{
    if(isSH3())
    {
      WDTSH3.WTCNT.WRITE = 0;
      WDTSH3.WTCNT.WRITE = 0x65;
      CPGSH3.FRQCR.word = 0x1000 | s->FRQCR;
      BSCSH3.CS0BCR.lword = s->CS0BCR;
      BSCSH3.CS0WCR.lword = s->CS0WCR;
      BSCSH3.CS2BCR.lword = s->CS2BCR;
      BSCSH3.CS2WCR.lword = s->CS2WCR;
      BSCSH3.CS3BCR.lword = s->CS3BCR;
      BSCSH3.CS3WCR.lword = s->CS3WCR;
      BSCSH3.CS5ABCR.lword = s->CS5aBCR;
      BSCSH3.CS5AWCR.lword = s->CS5aWCR;
    }
    else if(isSH4())
    {
      BSC.CS0WCR.WR = 11; /* 18 cycles */

      CPG.FLLFRQ.lword = s->FLLFRQ;
      CPG.FRQCR.lword = s->FRQCR;
      CPG.FRQCR.KICK = 1;
      while(CPG.LSTATS != 0) {}

      BSC.CS0BCR.lword = s->CS0BCR;
      BSC.CS0WCR.lword = s->CS0WCR;
      BSC.CS2BCR.lword = s->CS2BCR;
      BSC.CS2WCR.lword = s->CS2WCR;
      BSC.CS3BCR.lword = s->CS3BCR;
      BSC.CS3WCR.lword = s->CS3WCR;

      if(BSC.CS3WCR.A3CL == 1)
          *SDMR3_CL2 = 0;
      else
          *SDMR3_CL3 = 0;

      BSC.CS5ABCR.lword = s->CS5aBCR;
      BSC.CS5AWCR.lword = s->CS5aWCR;
    }
    else return;
}

/*settings for the fxcg50 / G90+E*/
static struct cpg_overclock_setting settings_fxcg50[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = 0x0F011112,
      .CS0BCR  = 0x36DA0400,
      .CS2BCR  = 0x36DA3400,
      .CS3BCR  = 0x36DB4400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x000003C0,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_8,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x00000340,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR  = (SH4_PLL_26x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_8,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x00000240,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR  = (SH4_PLL_32x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x000002C0,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR  = (SH4_PLL_26x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_8,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x00000440,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
};

/*settings for the prizm fxcg10/20*/
static struct cpg_overclock_setting settings_prizm[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = 0x0F102203,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_8<<20)+(SH4_DIV_16<<12)+(SH4_DIV_16<<8)+SH4_DIV_32,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x000100C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_32,
      .CS0BCR  = 0x24900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x000002C0,
      .CS2WCR  = 0x000201C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_32,
      .CS0BCR  = 0x44900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x00000440,
      .CS2WCR  = 0x00040340,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_26x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_16,
      .CS0BCR  = 0x34900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x000003C0,
      .CS2WCR  = 0x000402C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
};


/*settings for the fx9860G SH3 based*/
static struct cpg_overclock_setting settings_fx9860g_sh3[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0xFFFFFFFF,  // not used for SH3 MPUs
      .FRQCR   = 0x1001,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0xFFFFFFFF,  // not used for SH3 MPUs
      .FRQCR   = (SH3_PLL_2x<<8)+(SH3_DIV_1<<4)+SH3_DIV_2,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0xFFFFFFFF,  // not used for SH3 MPUs
      .FRQCR   = (SH3_PLL_3x<<8)+(SH3_DIV_1<<4)+SH3_DIV_3,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0xFFFFFFFF,  // not used for SH3 MPUs
      .FRQCR   = (SH3_PLL_4x<<8)+(SH3_DIV_1<<4)+SH3_DIV_4,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0xFFFFFFFF,  // not used for SH3 MPUs
      .FRQCR   = (SH3_PLL_4x<<8)+(SH3_DIV_1<<4)+SH3_DIV_4,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000000C0,
      .CS2WCR  = 0x000100C0,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
};

/*settings for the fx9860GII*/
static struct cpg_overclock_setting settings_fx9860g_sh4[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = 0x0F202203,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000005C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_8<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x24920200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00020140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_16,
      .CS0BCR  = 0x14900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000003C0,
      .CS2WCR  = 0x000302C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
};

/*settings for the fx9860GII-2 / G35+EII*/
static struct cpg_overclock_setting settings_g35pe2[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = 0x0F202203,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000005C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_8<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00020140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00031340 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x14900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00020140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00031340 },
};

static struct cpg_overclock_setting *get_settings(void)
{
    if(gint[HWCALC] == HWCALC_FXCG50)
        return settings_fxcg50;
    if(gint[HWCALC] == HWCALC_PRIZM)
        return settings_prizm;
    if(gint[HWCALC] == HWCALC_G35PE2)
        return settings_g35pe2;
    if(gint[HWCALC] == HWCALC_FX9860G_SH4)
        return settings_fx9860g_sh4;
    if(gint[HWCALC] == HWCALC_FX9860G_SH3)
        return settings_fx9860g_sh3;        
    return NULL;
}

int clock_get_speed(void)
{
    /* TODO : Add SH3 cases just hereafter*/
    if(!isSH4())
        return CLOCK_SPEED_UNKNOWN;
    

    /* All SH4-based FXCGs and FX9860Gs should be handled by this part */
    struct cpg_overclock_setting *settings = get_settings();
    if(!settings)
        return CLOCK_SPEED_UNKNOWN;

    for(int i = 0; i < 5; i++) {
        struct cpg_overclock_setting *s = &settings[i];

        if(CPG.FLLFRQ.lword == s->FLLFRQ
            && CPG.FRQCR.lword == s->FRQCR
            && BSC.CS0BCR.lword == s->CS0BCR
            && BSC.CS2BCR.lword == s->CS2BCR
            && BSC.CS3BCR.lword == s->CS3BCR
            && BSC.CS5ABCR.lword == s->CS5aBCR
            && BSC.CS0WCR.lword == s->CS0WCR
            && BSC.CS2WCR.lword == s->CS2WCR
            && BSC.CS3WCR.lword == s->CS3WCR
            && BSC.CS5AWCR.lword == s->CS5aWCR)
            return CLOCK_SPEED_F1 + i;
    }

    return CLOCK_SPEED_UNKNOWN;
}

void clock_set_speed(int level)
{
    if(level < CLOCK_SPEED_F1 || level > CLOCK_SPEED_F5)
        return;
    if(clock_get_speed() == level)
        return;

    struct cpg_overclock_setting *settings = get_settings();
    if(!settings)
        return;

    struct cpg_overclock_setting *s = &settings[level - CLOCK_SPEED_F1];
    uint32_t old_Pphi = clock_freq()->Pphi_f;

    /* Wait for asynchronous tasks to complete */
    gint_world_sync();

    /* Disable interrupts during the change */
    cpu_atomic_start();

    /* Set the clock settings */
    cpg_set_overclock_setting(s);

    /* Determine the change in frequency for Pϕ and recompute CPG data */
    cpg_compute_freq();
    uint32_t new_Pphi = clock_freq()->Pphi_f;

    /* Update timers' TCNT and TCOR to match the new clock speed */
    void timer_rescale(uint32_t old_Pphi, uint32_t new_Pphi);
    timer_rescale(old_Pphi, new_Pphi);

    cpu_atomic_end();
}