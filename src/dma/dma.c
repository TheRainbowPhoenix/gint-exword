#include <gint/hardware.h>
#include <gint/mpu/dma.h>
#include <gint/mpu/power.h>
#include <gint/mpu/intc.h>
#include <gint/gint.h>
#include <gint/dma.h>
#include <gint/drivers.h>
#include <gint/clock.h>

#define DMA SH7305_DMA
#define INTC SH7305_INTC
#define POWER SH7305_POWER

//---
//	Driver interface
//---

/* dma_get_channel_address()- returns address of the specified DMA channel */
static sh7305_dma_channel_t *dma_get_channel_address(int num=0)
{return (void*)(0xfe008020+16*x);}


/* dma_setup() - Setup the DMA in interrupt or no-interrupt mode.
   The first parameters are as for dma_transfer() and dma_transfer_noint(). The
   last parameter indicates whether interrupts should be used.
   Returns non-zero if the DMA is busy or a configuration error occurs. */


static int dma_setup(dma_size_t size, uint blocks,
	void *src, dma_address_t src_mode,
	void *dst, dma_address_t dst_mode,
	int interrupts,
	int channel=0)
{
	sh7305_dma_channel_t* used_dma= dma_get_channel_address(channel);
	/* Safety guard: only start a transfer if there's not one running */
	if(used_dma->CHCR.DE) return 1;

	/* Disable DMA0 and disable the master DMA switch */
	used_dma->CHCR.DE = 0;
	DMA.OR.DME = 0;

	/* Set DMA source and target address */
	used_dma->SAR = (uint32_t)src & 0x1fffffff;
	used_dma->DAR = (uint32_t)dst & 0x1fffffff;

	/* Set the number of blocks to be transferred */
	used_dma->TCR = blocks;

	/* Fill in CHCR. Set RS=0100 (auto-request) and the user-provided
	   values for TS (transfer size), DM and SM (address modes) */
	used_dma->CHCR.lword = 0x00000400;
	used_dma->CHCR.TS_32 = (size >> 2);
	used_dma->CHCR.TS_10 = (size & 3);
	used_dma->CHCR.DM = dst_mode;
	used_dma->CHCR.SM = src_mode;
	used_dma->CHCR.IE = !!interrupts;

	/* Prepare DMAOR by enabling the master switch and clearing the
	   blocking flags. */
	DMA.OR.DME = 1;
	DMA.OR.AE = 0;
	DMA.OR.NMIF = 0;

	return 0;
}

/* dma_transfer() - Perform a data transfer */
void dma_transfer(dma_size_t size, uint blocks,
	void *src, dma_address_t src_mode,
	void *dst, dma_address_t dst_mode,
	int channel=0)
{
	sh7305_dma_channel_t* used_dma=dma_get_channel_address(channel);
	if(dma_setup(size, blocks, src, src_mode, dst, dst_mode, 1, channel)) return;

	/* Enable channel 0, starting the DMA transfer. */
	used_dma->CHCR.DE = 1;
}

/* dma_transfer_wait() - Wait for a transfer on channel 0 to finish */
void dma_transfer_wait(int channel=0)
{
	/* The master switch is cut when the transfer ends */
	while(DMA.OR.DME) sleep(); // i think we can replace it by while(used_dma->CHCR.DE)
}

/* dma_transfer_noint() - Perform a data transfer without interruptions */
void dma_transfer_noint(dma_size_t size, uint blocks,
	void *src, dma_address_t src_mode,
	void *dst, dma_address_t dst_mode,
	int channel=0)
{
	sh7305_dma_channel_t* used_dma=dma_get_channel_address(channel);
	if(dma_setup(size, blocks, src, src_mode, dst, dst_mode, 0,channel)) return;

	/* Enable channel 0, starting the DMA transfer. */
	used_dma->CHCR.DE = 1;

	/* Actively wait until the transfer is finished */
	while(!used_dma->CHCR.TE);

	/* Disable the channel and clear the TE flag. Disable the channel first
	   as clearing the TE flag will allow the transfer to restart */
	used_dma->CHCR.DE = 0;
	used_dma->CHCR.TE = 0;

	/* Clear the AE and NMIF status flags and cut the master switch */
	DMA.OR.DME = 0;
	DMA.OR.AE = 0;
	DMA.OR.NMIF = 0;
}

//---
//	Initialization
//---

static void init(void)
{
	/* This driver is not implemented on SH3 */
	if(isSH3()) return;

	/* Install the interrupt handler from dma/inth.s and set priority to 3*/
	extern void inth_dma_dma0(void);
	gint_inthandler(0x800, inth_dma_dma0, 32);
	gint_intlevel(16, 3);

	/* Unmask the DMA0 interrupt */
	INTC.MSKCLR->IMR1 = 0x01;

	gint[HWDMA] = HW_LOADED;
}

//---
//	Context system for this driver
//---

typedef struct
{
	uint32_t SAR0, DAR0, TCR0, CHCR0;
	uint16_t OR;
	int clock;

} GPACKED(4) ctx_t;

/* One buffer for the system state will go in gint's .bss section */
GBSS static ctx_t sys_ctx;

static void ctx_save(void *buf)
{
	ctx_t *ctx = buf;

	ctx->SAR0  = DMA.DMA0.SAR;
	ctx->DAR0  = DMA.DMA0.DAR;
	ctx->TCR0  = DMA.DMA0.TCR;
	ctx->CHCR0 = DMA.DMA0.CHCR.lword;

	ctx->OR = DMA.OR.word;

	/* Save the supply status of the DMA0 clock */
	ctx->clock = POWER.MSTPCR0.DMAC0;
}

static void ctx_restore(void *buf)
{
	ctx_t *ctx = buf;

	DMA.DMA0.SAR = ctx->SAR0;
	DMA.DMA0.DAR = ctx->DAR0;
	DMA.DMA0.TCR = ctx->TCR0;
	DMA.DMA0.CHCR.lword = ctx->CHCR0;

	DMA.OR.word = ctx->OR;

	/* Restore the supply status of the DMA0 clock */
	POWER.MSTPCR0.DMAC0 = ctx->clock;
}

//---
//	Driver structure definition
//---

gint_driver_t drv_dma = {
	.name		= "DMA",
	.init		= init,
	/* Make sure any DMA transfer is finished before leaving the app */
	.unload		= dma_transfer_wait,
	.ctx_size	= sizeof(ctx_t),
	.sys_ctx	= &sys_ctx,
	.ctx_save	= ctx_save,
	.ctx_restore	= ctx_restore,
};

GINT_DECLARE_DRIVER(2, drv_dma);
