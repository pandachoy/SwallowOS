ENTRY (_start)

SECTIONS
{
    /* .text 段 */
    . = 0x400000;
    .text : {
        *(.text .text.*)
    }
    
    /* .rodata 段 */
    .rodata : {
        *(.rodata .rodata.*)
    }
    
    /* .data 段 */
    .data : {
        *(.data .data.*)
    }
    
    /* .bss 段 */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
}