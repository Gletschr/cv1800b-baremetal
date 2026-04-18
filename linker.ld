/*
 * CV1800B memory map (from datasheet section 1.5):
 *   - There is NO general-purpose SRAM for the main C906 core.
 *   - The Mask ROM (at 0x04400000) initialises the built-in DDR, then loads
 *     the FSBL binary into DDR starting at 0x80000000 and jumps there.
 *   - DDR is 64 MB: 0x80000000 – 0x83FFFFFF.
 *
 * Our binary replaces the FSBL, so it runs from 0x80000000 in DDR.
 * Stack is at load_base + 2 MB (set via auipc in startup.S).
 */
MEMORY {
    dram (rwx) : ORIGIN = 0x80000000, LENGTH = 64M
}

ENTRY(_start)

SECTIONS {
    . = ORIGIN(dram);

    .text : {
        *(.text.entry)
        *(.text*)
    } > dram

    .rodata : {
        *(.rodata*)
    } > dram

    /* No .data — avoid absolute-address relocations for initialized globals. */

    .bss (NOLOAD) : {
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        _bss_end = .;
    } > dram
}
