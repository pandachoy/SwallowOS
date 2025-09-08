.global get_to_ring3
.type get_to_ring3, @function
get_to_ring3:
    movl $(0x20 | 3), %eax             # user data Segment
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    mov $0xc0000080, %rcx
    rdmsr
    orl $1, %eax
    wrmsr
    mov $0xc0000081, %rcx      # enable   syscall/sysret in 64-bit mode
    rdmsr
    mov $0x00180008, %edx      # target code segment: Reads a non-NULL selector from IA32_STAR[63:48] + 16 stack segment IA32_STAR[63:48] + 8.
    wrmsr

    mov %rdi, %rcx             # to load into rip
    mov $0x002, %r11           # to load into eflags, no IF for test
    sysretq

.global set_ring0_msr
.type set_ring0_msr, @function
set_ring0_msr:
    mov $0xc0000080, %rcx
    rdmsr
    orl $1, %eax
    wrmsr

    mov $0xc0000081, %rcx
    rdmsr
    mov $0x00000008, %edx
    wrmsr

    mov %rdi, %rdx
    shr $32, %rdx
    mov %edi, %eax
    mov $0xc0000082, %rcx
    wrmsr
    ret

.global get_to_ring0
.type get_to_ring0, @function
get_to_ring0:
    syscall








