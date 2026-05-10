/*
	Linker script for fxCP add-ins linking with HollyHock3.
*/

OUTPUT_ARCH(sh4)
OUTPUT_FORMAT(elf32-sh)
ENTRY(_hhk3_entry)

MEMORY
{
	/* VRAM backup - our main fixed area to load code, size 320x528x2 */
	ram (rwx): o = 0x8c052800, l = 337920
	/* End-of-RAM area, 64 kB are safe to use according to experience. What
	   happens is HollyHock itself is loaded at 0x8cfd0000 (128 kB before the
	   application) so we have to fit after it. Might be possible to cut into the loader. */
	eram (rwx): o = 0x8cff0000, l = 0xef00 /* 64k - 0x1100 */
	/* Space for the vbr */
	vbr (rwx): o = 0x8cffef00, l = 0x1100
	/* On-chip IL memory */
	ilram (rwx):  o = 0xe5200000, l = 4k
	/* On-chip X and Y memory */
	xyram (rwx):  o = 0xe500e000, l = 16k
}

SECTIONS
{
	/*
	**  ROM sections
	*/

	/* First address to be mapped to ROM */
	_brom = ORIGIN(ram);
	/* Size of ROM mappings */
	_srom = SIZEOF(.text) + SIZEOF(.rodata)
	      + SIZEOF(.gint.drivers) + SIZEOF(.gint.blocks);

	/* Machine code going to ROM:
	   - Entry function (.text.entry)
	   - Compiler-provided constructors (.ctors) and destructors (.dtors)
	   - All text from .text and .text.* (including user code) */
	.hh3 : {
		*(.hh3*)
		*(.eram*)
	} > eram

	.note.hollyhock : {
	    *(.note.hollyhock)
	} > ram


	.text : {
		*(.text.entry)

		_bctors = . ;
		KEEP(*(.ctors .ctors.*))
		_ectors = . ;

		_bdtors = . ;
		KEEP(*(.dtors .dtors.*))
		_edtors = . ;

		. = ALIGN(0x10);
		_gint_exch_start = . ;
		*(.gint.exch)
		_gint_exch_end = . ;

		. = ALIGN(0x10);
		_gint_tlbh_start = . ;
		*(.gint.tlbh)
		_gint_tlbh_end = . ;

		*(.text .text.*)
	} > ram

	/* gint's interrupt handler blocks (.gint.blocks)
	   Although gint's blocks end up in VBR space, they are relocated at
	   startup by the library/drivers, so we store them here for now */
	.gint.blocks ALIGN(4) : ALIGN(4) {
		KEEP(*(.gint.blocks));
	} > ram

	/* Exposed driver interfaces (.gint.drivers)
	   The driver information is required to start and configure the
	   driver, even if the symbols are not referenced */
	.gint.drivers ALIGN(4) : ALIGN(4) {
		_gint_drivers = . ;
		KEEP(*(SORT_BY_NAME(.gint.drivers.*)));
		_gint_drivers_end = . ;
	} > ram

	/* Read-only data going to ROM:
	   - Resources or assets from fxconv or similar converters
	   - Data marked read-only by the compiler (.rodata and .rodata.*) */
	.rodata ALIGN(4) : ALIGN(4) SUBALIGN(4) {
		/* Put these first, they need to be 4-aligned */
		*(.rodata.4)

		*(.rodata .rodata.*)
	} > ram



	/*
	**  RAM sections
	*/

	/* Read-write data sections going to RAM (.data and .data.*) */
	.data ALIGN(4) : ALIGN(4) {
		_ldata = LOADADDR(.data);
		_rdata = . ;

		*(.data .data.*)
		/* Code that must remain mapped; no MMU on HIOS/CP, so fine */
		*(.gint.mapped)
		/* References to mapped code - no relocation needed */
		*(.gint.mappedrel)

		. = ALIGN(16);
	} > ram

	/* Read-write data sub-aligned to 4 bytes (mainly from fxconv) */
	.data.4 ALIGN(4) : ALIGN(4) SUBALIGN(4) {
		*(.data.4)
		. = ALIGN(16);
	} > ram

	_sdata = SIZEOF(.data) + SIZEOF(.data.4);

	/* On-chip memory sections: IL, X and Y memory */

	. = ORIGIN(ilram);
	.ilram ALIGN(4) : ALIGN(4) {
		_lilram = ABSOLUTE(LOADADDR(.ilram) + ORIGIN(ram));
		_rilram = . ;

		*(.ilram)

		. = ALIGN(16);
	} > ilram

	. = ORIGIN(xyram);
	.xyram ALIGN(4) : ALIGN(4) {
		_lxyram = ABSOLUTE(LOADADDR(.xyram) + ORIGIN(ram));
		_rxyram = . ;

		*(.xram .yram .xyram)

		. = ALIGN(16);

	} > xyram

	_silram = SIZEOF(.ilram);
	_sxyram = SIZEOF(.xyram);

	_lgmapped = ABSOLUTE(0);
	_sgmapped = ABSOLUTE(0);
	_lreloc = ABSOLUTE(0);
	_sreloc = ABSOLUTE(0);
	_gint_region_vbr = ORIGIN(vbr);

	/* BSS data going to RAM. The BSS section is to be stripped from the
	   ELF file later, and wiped by the loader */
	.bss : {
		_rbss = . ;

		*(.bss .bss.* COMMON)

		. = ALIGN(16);
	} > ram

	_sbss = SIZEOF(.bss);

	/* gint's uninitialized BSS section, going to static RAM. All the large
	   data arrays will be located here */
	.gint.bss : {
		*(.gint.bss)
		. = ALIGN(16);

		/* End of user RAM */
		_euram = . ;
	} > ram

	_sgbss = SIZEOF(.gint.bss);

	/*
	**  Unused sections
	*/

	/DISCARD/ : {
		/* SH3-only data sections */
		*(.gint.rodata.sh3 .gint.data.sh3 .gint.bss.sh3)
		/* Java class registration (why are they even here?!) */
		*(.jcr)
		/* Asynchronous unwind tables: no C++ exception handling */
		*(.eh_frame_hdr)
		*(.eh_frame)
		/* Comments or anything the compiler might generate */
		*(.comment)
	}
}
