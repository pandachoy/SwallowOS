/* Constants for multiboot header */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */


# Declare constants for GDT
# Access bits
.set PRESENT,  1<<7
.set NOT_SYS,  1<<4
.set EXEC,     1<<3
.set DC,       1<<2
.set RW,       1<<1
.set ACCESSED, 1<<0
# Flag bits
.set GRAN_4K,  1<<7
.set SZ_32,    1<<6

/* Multiboot header */
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Stack */
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/* Paging */
.section .lowbss, "aw", @nobits
.align 4096
page_directory:
	.skip 4096
first_page_table:
	.skip 4096

.section .lowdata, "aw"
gdt_base:
gdt_null:
        .skip 8
gdt_code:
        .long 0xffff                                       # Limit & Base (low, bits 0-15)
        .byte 0                                            # Base (mid, bits 16-23)
        .byte PRESENT | NOT_SYS | EXEC | RW                # Access
        .byte GRAN_4K | SZ_32 | 0xf                                # Flags & Limit
        .byte 0                                            # 
gdt_data:
        .long 0xffff
        .byte 0
        .byte PRESENT | NOT_SYS | RW
        .byte GRAN_4K | SZ_32 | 0xf
        .byte 0
gdt_tss:
        .long 0x00000068
        .long 0x00CF8900
gdt_ptr:
        .word gdt_ptr - gdt_base
        .long gdt_base


/* Main text section */
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:
	/* Initialize page table and page directory, notice boot_page_table1 in higher half section and this section in lower half */
        movl $(first_page_table), %edi # boot_page_table1在.bss，因为此时还未enable paging，且.bss的加载地址和虚拟地址不同，而.multiboot.data和.multiboot.text都未+0xC0000000，因此实际加载地址应该为(boot_page_table1 - 0xC0000000) 
        # First address to map is address 0.
        # TODO: Start at the first kernel page instead. Alternatively map the first
        #       1 MiB as it can be generally useful, and there's no need to
        #       specially map the VGA buffer.
        movl $0, %esi
        # Map 1023 pages. The 1024th will be the VGA text buffer.
        movl $1023, %ecx

1:
        # Only map the kernel.
        cmpl $_kernel_start, %esi # _kernel_start的定义在linker.ld内，注意_kernel_start地址还未+0xC0000000，
        jl 2f
        # cmpl $(_kernel_end - 0xC0000000), %esi # 因为加载地址和虚拟地址不同，所以要-0xC0000000，实际上在这一步做了地址转换
        # jge 3f
        cmpl $(first_page_table + 4096), %edi     # 这里不取_kernel_start的位置，而是取一个页目录项的范围，即页表地址为
        jge 3f

        # Map physical address as "present, writable". Note that this maps
        # .text and .rodata as writable. Mind security and map them as non-writable.
        movl %esi, %edx
        orl $0x003, %edx
        movl %edx, (%edi) # 把edx的值（被映射的地址，即加载地址）赋给edi所指地址（页表项）

2:
        # Size of page is 4096 bytes.
        addl $4096, %esi
        # Size of entries in boot_page_table1 is 4 bytes.
        addl $4, %edi
        # Loop to the next entry if we haven't finished.
        loop 1b

3:
        # Map VGA video memory to 0xC03FF000 as "present, writable".
        movl $(0x000B8000 | 0x003), first_page_table + 1023 * 4

        # Map the page table to both virtual addresses 0x00000000 and 0xC0000000.
        # 把页表加载地址（物理地址）写入页dictionary表，注意映射两次，个人理解是page enable之后，偏移0xC0000000与未偏移的都可以正确找到对应页表及对应section
        movl $(first_page_table + 0x003), page_directory + 0 # 该场景下，虚拟地址与加载地址相同的section会映射到此
        movl $(first_page_table + 0x003), page_directory + 768 * 4 # 该场景下，虚拟地址与加载地址不同（相差0xC0000000）的section会映射到此

        # Set cr3 to the address of the boot_page_directory.
        movl $(page_directory), %ecx
        movl %ecx, %cr3

        # Enable paging and the write-protect bit.
        movl %cr0, %ecx
        orl $0x80000000, %ecx
        movl %ecx, %cr0

        # Jump to higher half with an absolute jump. 
        lea 4f, %ecx
        jmp *%ecx

.section .text

4:
	# At this point, paging is fully set up and enabled.
	# 此时可以直接用虚拟地址了

	# Unmap the identity mapping as it is now unnecessary. 
	# movl $0, boot_page_directory + 0
	# 可以把之前在页dictionary表中的相同映射去掉，因为此时已经没有虚拟地址与加载地址相同的section了

	# Reload crc3 to force a TLB flush so the changes to take effect.
	# movl %cr3, %ecx
	# movl %ecx, %cr3
        lgdt gdt_ptr

        cli
        in $0x92, %al
        orb $0x02, %al
        out %al, $0x92


        movl %cr0, %eax
        orb  $0x01, %al
        movl %eax, %cr0

        jmp $(0x08),$.reload_cs
.reload_cs:
        movl $0x10, %eax
        movl %eax, %ds
        movl %eax, %es
        movl %eax, %fs
        movl %eax, %gs
        movl %eax, %ss


	# Set up the stack.
	movl $stack_top, %esp

	# Enter the high-level kernel.
	call kernel_main

	# Infinite loop if the system has nothing more to do.
	cli
1:	hlt
	jmp 1b