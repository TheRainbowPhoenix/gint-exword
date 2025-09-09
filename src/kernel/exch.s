.global _gint_exch
.section .gint.exch, "ax"
.align 4

_gint_exch:
	sts.l	pr, @-r15

	/* Get the first word of the gint hardware array (HWMPU). If it has the
	   last bit set, we're SH3 */
	mov.l	.gint, r0
	mov.l	@r0, r0
	tst	#1, r0
	mov.l	.expevt_sh4, r6
	mov.l	.tra_sh4, r7
	bt	syscall
	mov.l	.expevt_sh3, r6
	mov.l	.tra_sh3, r7

syscall:
	/* If this is trapa #31, save context and run the syscall interface. */
	mov.l	@r6, r4
	mov.w	.expevt_trapa, r5
	mov.l	@r7, r0
	cmp/eq	r4, r5
	bf	catch
	cmp/eq	#31*4, r0
	bf	catch

	add	#-20, r15
	mov.l	.syscall_entry_plus1, r0
	mov	r15, r4
	mov.l	.callback, r1
	mov.l	@r1, r1
	jsr	@r1
	mov.l	r0, @r4

	bra	end
	add	#20, r15

catch:
	/* Panic if the catcher is NULL */
	mov.l	.catcher, r0
	mov.l	r4, @-r15
	mov.l	@r0, r0
	tst	r0, r0
	bt	panic

	/* Set BL=0, IMASK=15 */
	stc	sr, r7
	mov.l	r7, @-r15
	mov.l	.SR_set_IMASK, r1
	or	r7, r1
	mov.l	.SR_clear_BL, r2
	and	r2, r1
	ldc	r1, sr

	/* Call the catcher and leave if it returns zero (exception handled) */
	jsr	@r0
	nop

	ldc.l	@r15+, sr
	tst	r0, r0
	bt.s	end
	mov.l	@r15+, r4

panic:
	/* RTE to the panic function, but manually, so that SPC is preserved */
	ldc	r4, r4_bank
	lds.l	@r15+, pr

	/* Here we switch banks so r0..r7 change meaning! */
	stc	ssr, r0
	ldc	r0, sr

	mov.l	.panic, r0
	mov.l	@r0, r0
	jmp	@r0
	nop

end:
	lds.l	@r15+, pr
	rte
	nop

.align 4

.gint:
	.long	_gint
.expevt_sh4:
	.long	0xff000024
.expevt_sh3:
	.long	0xffffffd4
.catcher:
	.long	_gint_exc_catcher
.panic:
	.long	_gint_exc_panic
.SR_set_IMASK:
	.long	(0xf << 4)
.SR_clear_BL:
	.long	~(1 << 28)
.callback:
	.long	_gint_inth_callback
.syscall_entry_plus1:
	.long	_syscall_entry + 1
.tra_sh3:
	.long	0xffffffd0
.tra_sh4:
	.long	0xff000020
.expevt_trapa:
	.word	0x160
