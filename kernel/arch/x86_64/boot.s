# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare constants for GDT
# Access bits
.set PRESENT,  1<<7
.set DPL_LOW,  1<<6 | 1<<5
.set NOT_SYS,  1<<4
.set EXEC,     1<<3
.set DC,       1<<2
.set RW,       1<<1
.set ACCESSED, 1<<0
# Flag bits
.set GRAN_4K,  1<<7
.set SZ_32,    1<<6
.set LONG_MODE,1<<5

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# Preallocate pages used for paging. Don't hard-code addresses and assume they
# are available, as the bootloader might have loaded its multiboot structures or
# modules there. This lets the bootloader know it must avoid the addresses.
.section .lowbss, "aw", @nobits
.code32
.align 4096
page_map_level4:
        .skip 4096                # size 512 * 8, above the same size
first_page_directory_ptr:
        .skip 4096
last_page_directory_ptr:
        .skip 4096
last_first_page_directory:    # 倒数第一的pdt
        .skip 4096
last_second_page_directory:   # 倒数第二的pdt
        .skip 4096
first_page_table:
        .skip 4096
second_page_table:
        .skip 4096

.section .lowdata, "aw"
.global tss_entry
.global tss_end
.align 16
tss_entry:
.long 0                            # 保留
.quad 0                            # rsp0 (内核栈指针，当用户态时发生中断，cpu会从此处取出内核栈指针)
.quad 0                            # rsp1 (不使用)
.quad 0                            # rsp2 (不使用)
.quad 0                            # 保留
.quad 0                            # IST1
.quad 0                            # IST2
.quad 0                            # IST3
.quad 0                            # IST4
.quad 0                            # IST5
.quad 0                            # IST6
.quad 0                            # IST7
.quad 0                            # 保留
.word 0                            # 保留
.word tss_end - tss_entry          # 如果不使用IOPB，可设置为tss的大小
tss_end:

.section .lowdata, "aw"
.code64
gdt_base:
gdt_null:
        .skip 8
gdt_code:
        .long 0xffff                                       # Limit & Base (low, bits 0-15)
        .byte 0                                            # Base (mid, bits 16-23)
        .byte PRESENT | NOT_SYS | EXEC | RW                # Access
        .byte GRAN_4K | LONG_MODE | 0xf                    # Flags & Limit
        .byte 0                                            # 
gdt_data:
        .long 0xffff
        .byte 0
        .byte PRESENT | NOT_SYS | RW
        .byte GRAN_4K | SZ_32 | 0xf
        .byte 0
gdt_user_code_32:                                          # 兼容，64位时不起作用
        .long 0xffff
        .byte 0
        .byte PRESENT | DPL_LOW | NOT_SYS | EXEC | RW
        .byte GRAN_4K | LONG_MODE | 0xf
        .byte 0
gdt_user_data:
        .long 0xffff
        .byte 0
        .byte PRESENT | DPL_LOW | NOT_SYS | RW
        .byte GRAN_4K | LONG_MODE | 0xf
        .byte 0
gdt_user_code:
        .long 0xffff
        .byte 0
        .byte PRESENT | DPL_LOW | NOT_SYS | EXEC | RW
        .byte GRAN_4K | LONG_MODE | 0xf
        .byte 0
gdt_tss:
gdt_tss_limit_1:
        .word 0                            # limit1
gdt_tss_base_1:
        .word 0                            # base1
gdt_tss_base_2:
        .byte 0                            # base2
gdt_tss_access:
        .byte 0x89                         # tss有自己的access byte定义
gdt_tss_limit_2:
        .byte 0                            # limit2, G=0, DB=0, L=0，由于tss在.lowdata
gdt_tss_base_3:
        .byte 0                            # base3
gdt_tss_base_4:
        .long 0                            # base4，由于tss在.lowdata 32位，此处可直接赋0
        .long 0                                            # 保留
gdt_ptr:
        .word gdt_ptr - gdt_base
        .long gdt_base

# Further page tables may be required if the kernel grows beyond 3 MiB.

# The kernel entry point.
.section .multiboot.text, "a"
.code32
.global _start
.type _start, @function
_start:
set_tss:
        # limit1
        movl $(tss_end - tss_entry), %eax
        movw %ax, gdt_tss_limit_1
        # base1
        movl $tss_entry, %eax
        movw %ax, gdt_tss_base_1
        # base2
        shr $16, %eax
        movb %al, gdt_tss_base_2
        # limit2，此处G、DB、L均为0
        movl $(tss_end - tss_entry), %eax
        shr $16, %eax
        movb %al, gdt_tss_limit_2
        # base3
        movl $tss_entry, %eax
        shr $24, %eax
        movb %al, gdt_tss_base_3

set_page:
        movl $first_page_table, %edi 
        movl $0, %esi
1:

        cmpl $(first_page_table + 4096 * 2), %edi     # 判断复制终点，这里复制两个pt的长度
        jge 3f

        # Map physical address as "present, writable". Note that this maps
        # .text and .rodata as writable. Mind security and map them as non-writable.
        movl %esi, %edx
        orl $0x007, %edx  # P、R/W、U/S
        movl %edx, (%edi) # 把edx的值（被映射的地址，即加载地址）赋给edi所指地址（页表项）

2:
        # Size of page is 4096 bytes.
        addl $4096, %esi
        # Size of entries in boot_page_table1 is 4 bytes.
        addl $8, %edi                                  # 注意，64位页表单个项长度为8
        # Loop to the next entry if we haven't finished.
        loop 1b

3:
        # Map VGA video memory to 0xC03FF000 as "present, writable".
        movl $(0x000B8000 | 0x007), second_page_table + 511 * 8
        movl $(first_page_table + 0x007), last_second_page_directory + 0 # 该场景下，虚拟地址与加载地址相同的section会映射到此
        movl $(second_page_table + 0x007), last_second_page_directory + 1 * 8

        # 同时映射一个没有offset的页表
        movl $(last_second_page_directory + 0x007), first_page_directory_ptr + 0

        # higher half 的页表，在最后一个pdpt的第510项
        movl $(last_second_page_directory + 0x007), last_page_directory_ptr + 510 * 8

        # pml4
        movl $(first_page_directory_ptr + 0x007), page_map_level4 + 0 * 8
        movl $(last_page_directory_ptr + 0x007), page_map_level4 + 511 * 8

        movl %cr4, %eax
        orl $0b100000, %eax
        movl %eax, %cr4

        movl $page_map_level4, %eax
        movl %eax, %cr3

        movl $0xC0000080, %ecx            # Set the c-register to 0xC0000080 which is the EFER MSR
        rdmsr                             # Read from the model-specific register
        orl $0x100, %eax                  # Set the LM-bit which is the 9th bit (bit 8)
        wrmsr

        mov %cr0, %eax                   # Set the A-register to control register 0
        orl $0x80000001, %eax             # Set the PG-bit, which is the 31nd bit, and the PM-bit, which is the 0th bit
        mov %eax, %cr0                   # Set control register 0 to the A-register

break:
        # lea real64 - 0xffffffff80000000, %eax
        # jmp *%eax
        lea compat, %ecx
        jmp *%ecx

.section .multiboot.text, "a"
.code32
compat:
        lgdt gdt_ptr
load_tss:
        movw $0x30, %ax
        ltr %ax

        jmp $0x08, $(real64 - 0xffffffff80000000)


        # movl $(boot_page_table1 + 0x003), boot_page_directory + 768 * 4 # 该场景下，虚拟地址与加载地址不同（相差0xC0000000）的section会映射到此

        # # Set cr3 to the address of the boot_page_directory.
        # movl $(boot_page_directory), %ecx
        # movl %ecx, %cr3

        # # Enable paging and the write-protect bit.
        # movl %cr0, %ecx
        # orl $0x80000000, %ecx
        # movl %ecx, %cr0

        # # Jump to higher half with an absolute jump. 
        # lea 4f, %ecx
        # jmp *%ecx

.section .text
.code64
real64:
        mov $real64_1, %rcx
        jmp *%rcx

.section .text
.code64
real64_1:
        jmp real64_2
real64_2:
        mov $0x10, %eax
    mov %eax, %ds
    mov %eax, %es
    mov %eax, %fs
    mov %eax, %gs
    mov %eax, %ss

        # At this point, paging is fully set up and enabled.
        # 此时可以直接用虚拟地址了

        # Unmap the identity mapping as it is now unnecessary. 
        # movl $0, boot_page_directory + 0xC0000000 + 0
        # 可以把之前在页dictionary表中的相同映射去掉，因为此时已经没有虚拟地址与加载地址相同的section了

        # Reload crc3 to force a TLB flush so the changes to take effect.
        # movl %cr3, %ecx
        # movl %ecx, %cr3

        # Set up the stack.
        mov $stack_top, %rsp

        # Enter the high-level kernel.
        callq kernel_main

        # Infinite loop if the system has nothing more to do.
        cli
1:      hlt
        jmp 1b