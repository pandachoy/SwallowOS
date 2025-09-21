.global do_syscall
.type do_syscall, @function
.align 4
do_syscall:
    /* load kernel segment reg */
    movl $0x10, %ebx
    movw %bx, %ds
    movw %bx, %es
    movw %bx, %fs
    movw %bx, %gs

    /* save user stack */
    mov current_task_TCB(%rip), %rbx
    # mov (%rbx), %rbx
    mov TCB_rsp_offset(%rip), %r12
    # mov (%r12), %r12
    # add %r12, %rbx
    mov %rsp, (%rbx, %r12, 1)

    /* load kernel stack */
    mov TCB_rsp0_offset(%rip), %r12
    mov (%rbx, %r12, 1), %rsp

    /* syscall number is stored in rax */
    push %rcx
    mov %r10, %rcx
    mov %rax, %rbx
    shl $3, %rbx
    mov $syscalls, %r12
    add %r12, %rbx
    /* syscall result will be stored in rax */
    call (%rbx)
    pop %rcx

    /* save kernel stack */
    mov current_task_TCB(%rip), %rbx
    mov TCB_rsp0_offset(%rip), %r12
    mov %rsp, (%rbx, %r12, 1)

    /* load user stack */
    mov TCB_rsp_offset(%rip), %r12
    mov (%rbx, %r12, 1), %rsp

    /* restore user regs */
    movl $(0x20 | 3), %ebx             # user data Segment
    movw %bx, %ds
    movw %bx, %es
    movw %bx, %fs
    movw %bx, %gs

    # mov $0x002, %r11                   # switch IF for now
    /* rip already stored in rcx */
    sysretq