ENTRY (_start)

SECTIONS
{
        . = 0x00200000;

        _kernel_start = .;
                /* 加载地址和虚拟地址相同 */
        .multiboot.data : {
            *(.multiboot.data)
        }

       .multiboot.text : {
           *(.multiboot.text)
       }
           
           .lowdata : {
                   *(.lowdata)
                   *(.lowbss)
                   *(.tm_clone_table)
           }

        . += 0xffffffff80000000;
        /* Add a symbol that indicates the start address of the kernel. */
        /* 加载地址和虚拟地址不同 */
        .text ALIGN (4K) : AT (ADDR (.text) - 0xffffffff80000000)
        {
                *(.text)
        }
        .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xffffffff80000000)
        {
                *(.rodata)
        }
        .data ALIGN (4K) : AT (ADDR (.data) - 0xffffffff80000000)
        {
                *(.data)
        }
        .bss ALIGN (4K) : AT (ADDR (.bss) - 0xffffffff80000000)
        {
                *(COMMON)
                *(.bss)
                *(.bootstrap_stack)
        }
        /* Add a symbol that indicates the end address of the kernel. */
        _kernel_end = .;
}