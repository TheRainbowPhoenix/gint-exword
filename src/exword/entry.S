.section .text.exword.entry,"ax"
.global _exword_entry

_exword_entry:
    mov.l .save_end, r1
    stc.l gbr, @-r1
    sts.l pr, @-r1
    mov.l r15, @-r1
    mov.l r14, @-r1
    mov.l r13, @-r1
    mov.l r12, @-r1
    mov.l r11, @-r1
    mov.l r10, @-r1
    mov.l r9, @-r1
    mov.l r8, @-r1

    mov.l .ram, r4
    mov.b @r4, r0
    mov.l .translate, r1
    jsr @r1
    mov #0, r5
    mov #-1, r1
    cmp/eq r1, r0
    bt .failed
    mov.l .p1, r1
    or r1, r0
    mov r0, r5
    mov.l .stack_offset, r1
    add r1, r0
    mov r0, r15
    mov.l .start, r1
    mov #0, r4
    jsr @r1
    nop
    bra .restore
    nop
.failed:
    mov.l .start, r1
    mov #-1, r4
    jsr @r1
    mov #0, r5
.restore:
    mov.l .save_start, r1
    mov.l @r1+, r8
    mov.l @r1+, r9
    mov.l @r1+, r10
    mov.l @r1+, r11
    mov.l @r1+, r12
    mov.l @r1+, r13
    mov.l @r1+, r14
    mov.l @r1+, r15
    lds.l @r1+, pr
    ldc.l @r1+, gbr
    rts
    nop

.balign 4
.save_end: .long 0x7400004c
.save_start: .long 0x74000024
.ram: .long 0x74000000
.translate: .long _mmu_translate
.p1: .long 0x80000000
.stack_offset: .long _gint_stack_end - 0x74000000
.start: .long _start
