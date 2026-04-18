/*
 * startup.c — bare-metal entry point for the CV1800B / T-Head C906.
 *
 * Boot flow:
 *   Mask ROM (in silicon)
 *     → reads fip.bin from the FAT32 microSD partition
 *     → loads our binary into DDR at 0x80000000
 *     → jumps to 0x80000000   ← _start begins here
 *         → sets stack pointer (two asm instructions, see _start below)
 *         → jumps to _c_init
 *             → zeros the .bss segment
 *             → calls main()
 *             → halts forever if main() ever returns
 */

/* BSS boundaries are defined by the linker script (linker.ld). */
extern char _bss_start[];
extern char _bss_end[];

/* Application entry point, defined in main.c. */
extern void main(void);

/*
 * clear_bss — zero the .bss segment before any C code runs.
 *
 * The Mask ROM does not guarantee that DDR is zeroed before handing control
 * to us, so uninitialised static/global variables would hold garbage without
 * this step.
 */
static void clear_bss(void)
{
    char *p = _bss_start;
    while (p < _bss_end)
        *p++ = 0;
}

/*
 * _c_init — C-level startup, entered from _start once the stack is valid.
 *
 * Performs the minimum work required before main() can run, then calls it.
 * Halts the CPU with wfi (wait-for-interrupt) if main() ever returns — there
 * is no OS to return to.
 */
void _c_init(void)
{
    clear_bss();
    main();
    for (;;)
        __asm__ volatile("wfi");
}

/*
 * _start — hardware reset vector.
 *
 * Placed first in the binary via the ".text.entry" section so that offset 0
 * (where the Mask ROM jumps) always lands here.
 *
 * The C906 arrives with an undefined stack pointer — no C code can run until
 * sp is valid.  We use two assembly instructions and nothing else (naked
 * suppresses the compiler prologue/epilogue that would require a working stack):
 *
 *   auipc sp, 512   sp = PC + 512 × 4096 = PC + 2 MB
 *                   Places the stack 2 MB above our load address (0x80200000),
 *                   well inside the 64 MB DDR window.  auipc is PC-relative, so
 *                   this is correct regardless of the actual load address.
 *
 *   j _c_init       Unconditional jump to _c_init; return address not needed.
 */
__attribute__((naked, section(".text.entry")))
void _start(void)
{
    __asm__(
        "auipc sp, 512\n\t"
        "j     _c_init"
    );
}
