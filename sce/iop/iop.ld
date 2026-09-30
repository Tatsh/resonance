/*
 * Relocatable link of an IOP module before the fixup step converts it to an IRX. The fixup tool
 * places the segments and creates the boundary symbols. The script only merges the input sections
 * in the order the shipped modules use and drops sections the loader does not read.
 */

OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(mips)
ENTRY(start)

SECTIONS
{
    .text : { *(.text) *(.text.*) }
    .rodata : { *(.rodata) *(.rodata.*) }
    .data : { *(.data) *(.data.*) }
    .sdata : { *(.sdata) *(.sdata.*) }
    .sbss : { *(.sbss) *(.sbss.*) *(.scommon) }
    .bss : { *(.bss) *(.bss.*) *(COMMON) }

    /DISCARD/ : {
        *(.comment)
        *(.pdr)
        *(.mdebug.*)
        *(.gnu.attributes)
        *(.reginfo)
        *(.MIPS.abiflags)
    }
}
