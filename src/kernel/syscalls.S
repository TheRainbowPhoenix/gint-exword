/*
**	gint:core:syscalls - calls to CASIOWIN
**
**	This file can be seen as a list of everywhere gint relies on the
**	underlying OS. Although I wish to make gint free-standing, there are
**	still a few hard questions, namely:
**	* MMU management, because doing it wrong might break the calculator.
**	* Dynamic allocation, because we can't trash the system heap.
**	* File system, because it's a mess and we might ruin the ROM.
*/

#include <gint/config.h>

.text

/* Dynamic allocation */
.global ___malloc
.global ___free
.global ___calloc
.global ___realloc

/* Bfile driver */
.global	_BFile_Remove
.global	_BFile_Rename
.global	_BFile_Create
.global	_BFile_Open
.global	_BFile_Close
.global	_BFile_Size
.global	_BFile_Seek
.global	_BFile_GetPos
.global	_BFile_Write
.global	_BFile_Read
.global	_BFile_FindFirst
.global	_BFile_FindNext
.global	_BFile_FindClose

/* Return to menu */
.global ___Timer_Install
.global ___Timer_Start
.global ___Timer_Stop
.global ___Timer_Deinstall
.global ___PutKeyCode
.global ___GetKeyWait
.global ___ClearKeyBuffer
.global ___GetVRAMAddress
.global ___ConfigureStatusArea
.global ___SetQuitHandler
/* VRAM backup on fx-CP */
.global ___VRAMBackup
.global ___VRAMRestore

/* Reset */
.global ___PowerOff
.global ___Reset

#define syscall_(id, syscall_table)	\
	mov.l	syscall_table, r2	;\
	mov.l	1f, r0			;\
	jmp	@r2			;\
	nop				;\
.balign 4				;\
1:	.long	id

#define syscall(id) syscall_(id, syscall_table)

#define fixed(address)			\
	mov.l	1f, r0			;\
	jmp	@r0			;\
	nop				;\
.balign 4				;\
1:	.long	address

#if GINT_OS_FX

/* Dynamic allocation */

___malloc:
	syscall(0x0acd)
___free:
	syscall(0x0acc)
___realloc:
	syscall(0x0e6d)

/* BFile driver */

_BFile_Remove:
	mov #0, r5
	syscall(0x0439)
_BFile_Create:
	syscall(0x0434)
_BFile_Open:
	syscall(0x042c)
_BFile_Close:
	syscall(0x042d)
_BFile_Size:
	syscall(0x042f)
_BFile_Seek:
	syscall(0x0431)
_BFile_GetPos:
	rts
	mov	#-1, r0
_BFile_Write:
	syscall(0x0435)
_BFile_Read:
	syscall(0x0432)
_BFile_FindFirst:
    syscall(0x043b)
_BFile_FindNext:
    syscall(0x043c)
_BFile_FindClose:
    syscall(0x043d)

/* Return to menu */

___Timer_Install:
	syscall(0x118)
___Timer_Start:
	syscall(0x11a)
___Timer_Stop:
	syscall(0x11b)
___Timer_Deinstall:
	syscall(0x119)
___PutKeyCode:
	syscall(0x248)
___GetKeyWait:
	syscall(0x247)
___ClearKeyBuffer:
	syscall(0x241)
___GetVRAMAddress:
	syscall(0x135)
___SetQuitHandler:
	syscall(0x494)

/* Reset */

___PowerOff:
	syscall(0x3f4)
___Reset:
	syscall(0x236)

syscall_table:
	.long	0x80010070

#endif /* GINT_OS_FX */

#if GINT_OS_CG
#define CASIOWIN_API_VERSIONS 2
.global _gint_set_CASIOWIN_API
.global _gint_get_CASIOWIN_API

/* System for dynamically selecting between the syscall and fixed version of
   each function based on the OS version.
   @r0: Internal function ID (from table below) */
_CASIOWIN_call:
	mov	#CASIOWIN_API_VERSIONS, r1
	mulu.w	r0, r1
	mov.l	3f, r0
	mov.l	@r0, r2
	sts	macl, r1
	add	r2, r1
	shll2	r1

	/* API version 0 is the normal syscall table */
	tst	r2, r2
	mova	.CASIOWIN_TABLE, r0
	bt.s	.syscall
	mov.l	@(r0, r1), r0

	/* Other API versions are the direct calls */
	tst	r0, r0
	bt	.missingCall

	jmp	@r0
	nop

.missingCall:
	mov.l	.gint_panic, r0
	mov.w	2f, r4
	jmp	@r0
	nop

.syscall:
	mov.l	1f, r1
	jmp	@r1
	nop

2:	.word	0x10c0
.balign 4
1:	.long	0x80020070
.gint_panic:
	.long	_gint_panic

_gint_set_CASIOWIN_API:
	mov.l	3f, r0
	rts
	mov.l	r4, @r0

_gint_get_CASIOWIN_API:
	mov.l	3f, r0
	rts
	mov.l	@r0, r0

.balign 4
3:	.long	.CASIOWIN_API

.CASIOWIN_TABLE:
	.long 0x1f44, 0x8025e0fc  /* malloc */
	.long 0x1f42, 0x8025dec8  /* free */
	.long 0x1f46, 0x8025ec3c  /* realloc */
	.long 0x1db4, 0x802404d2  /* BFile_Remove */
	.long 0x1db3, 0x80240482  /* BFile_Rename */
	.long 0x1dae, 0x802401b0  /* BFile_Create */
	.long 0x1da3, 0x8023fb90  /* BFile_Open */
	.long 0x1da4, 0x8023fd0e  /* BFile_Close */
	.long 0x1da6, 0x8023fdc4  /* BFile_Size */
	.long 0x1da9, 0x8023ff2c  /* BFile_Seek */
	.long 0x1dab, 0x8024003c  /* BFile_GetPos */
	.long 0x1daf, 0x8024025e  /* BFile_Write */
	.long 0x1dac, 0x80240082  /* BFile_Read */
	.long 0x1db6, 0x80240888  /* BFile_FindFirst */
	.long 0x1db8, 0x80240b06  /* BFile_FindNext */
	.long 0x1dba, 0x80240c10  /* BFile_FindClose */
	.long 0x08d9, 0x800b130c  /* Timer_Install */
	.long 0x08db, 0x800b1456  /* Timer_Start */
	.long 0x08dc, 0x800b14b2  /* Timer_Stop */
	.long 0x08da, 0x800b13d4  /* Timer_Deinstall */
	.long 0x12c6, 0           /* PutKeyCode */
	.long 0x12bf, 0x8017be56  /* GetKeyWait */
	.long 0x12c7, 0           /* ClearKeyBuffer */
	.long 0x01e6, 0x8004579a  /* GetVRAMAddress */
	.long 0x1e6e, 0           /* SetQuitHandler */
	.long 0x1839, 0           /* PowerOff */
	.long 0x1187, 0           /* Reset */

#define casiowin_call(id) bra _CASIOWIN_call; mov id, r0

___malloc:               casiowin_call(#0)
___free:                 casiowin_call(#1)
___realloc:              casiowin_call(#2)
_BFile_Remove:           casiowin_call(#3)
_BFile_Rename:           casiowin_call(#4)
_BFile_Create:           casiowin_call(#5)
_BFile_Open:             mov #0, r6; casiowin_call(#6)
_BFile_Close:            casiowin_call(#7)
_BFile_Size:             casiowin_call(#8)
_BFile_Seek:             casiowin_call(#9)
_BFile_GetPos:           casiowin_call(#10)
_BFile_Write:            casiowin_call(#11)
_BFile_Read:             casiowin_call(#12)
_BFile_FindFirst:        casiowin_call(#13)
_BFile_FindNext:         casiowin_call(#14)
_BFile_FindClose:        casiowin_call(#15)
___Timer_Install:        casiowin_call(#16)
___Timer_Start:          casiowin_call(#17)
___Timer_Stop:           casiowin_call(#18)
___Timer_Deinstall:      casiowin_call(#19)
___PutKeyCode:           casiowin_call(#20)
___GetKeyWait:           casiowin_call(#21)
___ClearKeyBuffer:       casiowin_call(#22)
___GetVRAMAddress:       casiowin_call(#23)
___ConfigureStatusArea:  casiowin_call(#24)
___SetQuitHandler:       casiowin_call(#25)
___PowerOff:             casiowin_call(#26)
___Reset:                casiowin_call(#27)

.data
.CASIOWIN_API:
	.long 0

#endif /* GINT_OS_CG */

#if GINT_OS_CP

___malloc:
	fixed(0x800cfb00)
___realloc:
	// TODO: realloc for fx-CP?
	rts
	mov	#0, r0
___free:
	fixed(0x800a76fc)

___GetVRAMAddress:
	fixed(0x8002e154)
___VRAMBackup:
	fixed(0x8002d3fa)
___VRAMRestore:
	fixed(0x8002d41a)

___Reset:
	fixed(0xa0000000)

#endif /* GINT_OS_CP */
