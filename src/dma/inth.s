/*
**  gint:dma:inth - Interrupt handler for the DMA
**  An easy one, just clears some flags and marks all transfers as finished.
*/

/* DMA TRANSFER ENDED INTERRUPT HANDLER - BYTES */


.global _inth_dma_dma0
_inth_dma_dma0:
    /* Clear the TE flag and DMA Enable in CHCR */
    mov.l    1f, r1
    mov.l    @r1, r0
    mov    #-4, r2
    and    r2, r0
    mov.l    r0, @r1

    /* Clear the AE and NMIF flags in OR, and cut the master switch */
    add    #0x34, r1
    mov.w    @r1, r0
    shlr8    r0
    shll8    r0
    mov.w    r0, @r1

    rte
    nop
    nop
    nop



.global _inth_dma_dma1
_inth_dma_dma1:
    /* Clear the TE flag and DMA Enable in CHCR */
    mov.l    2f, r1
    mov.l    @r1, r0
    mov    #-4, r2
    and    r2, r0
    mov.l    r0, @r1

    /* Clear the AE and NMIF flags in OR, and cut the master switch */
    add    #0x34, r1
    mov.w    @r1, r0
    shlr8    r0
    shll8    r0
    mov.w    r0, @r1

    rte
    nop
    nop
    nop



.global _inth_dma_dma2
_inth_dma_dma2:
    /* Clear the TE flag and DMA Enable in CHCR */
    mov.l    3f, r1
    mov.l    @r1, r0
    mov    #-4, r2
    and    r2, r0
    mov.l    r0, @r1

    /* Clear the AE and NMIF flags in OR, and cut the master switch */
    add    #0x34, r1
    mov.w    @r1, r0
    shlr8    r0
    shll8    r0
    mov.w    r0, @r1

    rte
    nop
    nop
    nop


1:    .long 0xfe00802c    /* CHCR0 - OR is 0x34 bytes after this */
2:    .long 0xfe00803c
3:    .long 0xfe00804c