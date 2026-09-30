/*
 * Link script of the Emotion Engine executable. The sections follow the order of the shipped
 * executable: code at 0x00100000, the VU microprograms, data, read-only data, the exception tables
 * and constructor lists, then the uninitialised data. crt0.s reads the boundary symbols defined
 * here. The unwind tables and the .init and .fini code come from the compiler runtime.
 */

ENTRY(_start)

PHDRS {
    text PT_LOAD;
}

SECTIONS {
    .text 0x00100000 : {
        _ftext = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t*)
        KEEP(*(.init))
        KEEP(*(.fini))
    } :text
    PROVIDE(_etext = .);
    PROVIDE(etext = .);

    .vudata ALIGN(16) : {
        *(.vudata)
        *(.vutext)
    }

    .data ALIGN(64) : {
        _fdata = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)
    }

    .rodata ALIGN(16) : {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r*)
    }

    .gcc_except_table ALIGN(16) : {
        *(.gcc_except_table)
        *(.gcc_except_table.*)
    }

    .eh_frame ALIGN(4) : {
        KEEP(*(.eh_frame))
    }

    .ctors ALIGN(4) : {
        KEEP(*crtbegin*.o(.ctors))
        KEEP(*(EXCLUDE_FILE(*crtend*.o) .ctors))
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    }

    .dtors ALIGN(4) : {
        KEEP(*crtbegin*.o(.dtors))
        KEEP(*(EXCLUDE_FILE(*crtend*.o) .dtors))
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    }

    _gp = ALIGN(16) + 0x7ff0;
    .sdata ALIGN(16) : {
        *(.sdata)
        *(.sdata.*)
        *(.lit4)
        *(.lit8)
    }
    _edata = .;
    PROVIDE(edata = .);

    .sbss ALIGN(16) : {
        _fbss = .;
        *(.sbss)
        *(.sbss.*)
        *(.scommon)
    }

    .bss ALIGN(64) : {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b*)
        *(COMMON)
    }
    . = ALIGN(16);
    _end = .;
    PROVIDE(end = .);

    /DISCARD/ : {
        *(.MIPS.abiflags)
        *(.comment)
        *(.pdr)
    }

    /* The heap runs from _end to the stack, as SetupHeap() is told by a size of -1. */
    PROVIDE(_heap_size = -1);
}
