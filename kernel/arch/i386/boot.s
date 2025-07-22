/* Constants for multiboot header */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* Multiboot header */
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/* Stack */
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/* Paging */
.section .bss, "aw", @nobits
	.align 4096
boot_page_directory:
	.skip 4096
boot_page_table1:
	.skip 4096

/* Main text section */
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:
	/* Initialize page table and page directory, notice boot_page_table1 in higher half section and this section in lower half */
	movl $(boot_page_table1 - 0xC0000000), %edi # boot_page_table1在.bss，因为此时还未enable paging，且.bss的加载地址和虚拟地址不同，而.multiboot.data和.multiboot.text都未+0xC0000000，因此实际加载地址应该为(boot_page_table1 - 0xC0000000) 
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
	cmpl $(_kernel_end - 0xC0000000), %esi # 因为加载地址和虚拟地址不同，所以要-0xC0000000，实际上在这一步做了地址转换
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
	movl $(0x000B8000 | 0x003), boot_page_table1 - 0xC0000000 + 1023 * 4

	# Map the page table to both virtual addresses 0x00000000 and 0xC0000000.
	# 把页表加载地址（物理地址）写入页dictionary表，注意映射两次，个人理解是page enable之后，偏移0xC0000000与未偏移的都可以正确找到对应页表及对应section
	movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 0 # 该场景下，虚拟地址与加载地址相同的section会映射到此
	movl $(boot_page_table1 - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 768 * 4 # 该场景下，虚拟地址与加载地址不同（相差0xC0000000）的section会映射到此

	# Set cr3 to the address of the boot_page_directory.
	movl $(boot_page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	# Enable paging and the write-protect bit.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump. 
	lea 4f, %ecx
	jmp *%ecx

.section .text

4:
	# At this point, paging is fully set up and enabled.
	# 此时可以直接用虚拟地址了

	# Unmap the identity mapping as it is now unnecessary. 
	movl $0, boot_page_directory + 0
	# 可以把之前在页dictionary表中的相同映射去掉，因为此时已经没有虚拟地址与加载地址相同的section了

	# Reload crc3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp

	# Enter the high-level kernel.
	call kernel_main

	# Infinite loop if the system has nothing more to do.
	cli
1:	hlt
	jmp 1b