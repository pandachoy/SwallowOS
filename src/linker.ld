ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final kernel image.*/
SECTIONS
{
    /* start from 2M */
    . = 2M;

    /* Multiboot header */
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data. */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    /* Read-write data (uninitialized) and stack */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /* Other sections if exist */
}