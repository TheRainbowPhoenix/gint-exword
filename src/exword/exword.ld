OUTPUT_ARCH(sh4)
OUTPUT_FORMAT(elf32-sh)
ENTRY(_exword_entry)

MEMORY
{
    rom (rx) : ORIGIN = 0x70000080, LENGTH = 0x1ff80
    vbr (rw) : ORIGIN = 0x74000100, LENGTH = 0x1100
    mapped (rw) : ORIGIN = 0x74001200, LENGTH = 0x400
    ram (rw) : ORIGIN = 0x74001600, LENGTH = 0x4a00
    stack (rw) : ORIGIN = 0x74006000, LENGTH = 0x2000
}

PHDRS
{
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    _brom = 0x70000000;
    _gint_region_vbr = ORIGIN(vbr);
    _gint_region_mapped = ORIGIN(mapped);
    _gint_heap_end = ORIGIN(stack);
    _gint_stack_end = ORIGIN(stack) + LENGTH(stack);

    .text : {
        _stext = .;
        KEEP(*(.text.exword.entry))
        *(.text.entry)
        . = ALIGN(4);
        _bctors = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array .ctors .ctors.*))
        _ectors = .;
        _bdtors = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
        KEEP(*(.fini_array .dtors .dtors.*))
        _edtors = .;
        . = ALIGN(16);
        _gint_exch_start = .;
        KEEP(*(.gint.exch))
        _gint_exch_end = .;
        . = ALIGN(16);
        _gint_tlbh_start = .;
        KEEP(*(.gint.tlbh))
        _gint_tlbh_end = .;
        . = ALIGN(16);
        _lgmapped = .;
        *(.gint.mapped)
        . = ALIGN(16);
        _sgmapped = ABSOLUTE(. - _lgmapped);
        *(.text .text.*)
        . = ALIGN(4);
        KEEP(*(.gint.blocks))
        . = ALIGN(4);
        _gint_drivers = .;
        KEEP(*(SORT_BY_NAME(.gint.drivers.*)))
        _gint_drivers_end = .;
        . = ALIGN(4);
        *(.rodata.4)
        *(.rodata .rodata.*)
        . = ALIGN(16);
        _etext = .;
    } > rom :text

    .bss (NOLOAD) : {
        _rbss = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(16);
    } > ram :NONE
    _sbss = SIZEOF(.bss);

    .data : AT(_etext) {
        _rdata = .;
        _lreloc = .;
        *(.gint.mappedrel)
        _sreloc = ABSOLUTE(. - _lreloc);
        *(.data .data.*)
        *(.got .got.*)
        . = ALIGN(16);
        _edata = .;
    } > ram :data
    _ldata = LOADADDR(.data);
    _sdata = SIZEOF(.data);
    _srom = _ldata + _sdata - _brom;

    .gint.bss (NOLOAD) : {
        *(.gint.bss)
        . = ALIGN(16);
        _euram = .;
    } > ram :NONE
    _sgbss = SIZEOF(.gint.bss);

    .onchip : {
        *(.ilram .xram .yram .xyram)
    } > rom :text
    _lilram = 0; _silram = 0; _rilram = 0;
    _lxyram = 0; _sxyram = 0; _rxyram = 0;

    /DISCARD/ : {
        *(.gint.rodata.sh3 .gint.data.sh3 .gint.bss.sh3)
        *(.comment .note* .eh_frame* .jcr .gnu.attributes)
    }

    ASSERT(_exword_entry == ORIGIN(rom), "D01 entry must be at 0x70000080")
    ASSERT(_srom <= 0x20000, "D01 image exceeds the loader's 128 KiB buffer")
    ASSERT(_euram + 4096 <= _gint_heap_end, "EX-word needs at least 4 KiB of heap")
    ASSERT(_sgmapped <= LENGTH(mapped), "EX-word mapped callback region overflow")
    ASSERT(_gint_exch_end - _gint_exch_start <= 0x300, "EX-word exception handler overflow")
    ASSERT(_gint_tlbh_end - _gint_tlbh_start <= 0x200, "EX-word TLB handler overflow")
    ASSERT(SIZEOF(.onchip) == 0, "EX-word on-chip sections need explicit OS state preservation")
}
