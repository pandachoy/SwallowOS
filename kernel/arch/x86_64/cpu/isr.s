.macro isr_no_err_stub p
isr_stub_\p:
    pushq $0                 # fake error code -> uniform stack layout
    pushq $\p                # vector number
    jmp exception_common
.endm

.macro isr_err_stub p
isr_stub_\p:
    pushq $\p                # vector number (real error code already on stack)
    jmp exception_common
.endm

.include "arch/x86_64/cpu/cpu.inc"

.section .text
exception_common:
    movq 0(%rsp), %rdi       # vector
    movq 8(%rsp), %rsi       # error code
    leaq 16(%rsp), %rdx      # pointer to iretq frame (rip, cs, rflags, rsp, ss)
    call exception_handler
    popq %rax                # discard vector
    popq %rax                # discard error code
    iretq

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13

isr_stub_14:
    mov (%rsp), %rdi
    lea 8(%rsp), %rsi
    push_all
    call page_fault_handler
    pop_all
    add $8, %rsp
    iretq

isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

.section .data
.global isr_stub_table
isr_stub_table:
    .quad isr_stub_0
    .quad isr_stub_1
    .quad isr_stub_2
    .quad isr_stub_3
    .quad isr_stub_4
    .quad isr_stub_5
    .quad isr_stub_6
    .quad isr_stub_7
    .quad isr_stub_8
    .quad isr_stub_9
    .quad isr_stub_10
    .quad isr_stub_11
    .quad isr_stub_12
    .quad isr_stub_13
    .quad isr_stub_14
    .quad isr_stub_15
    .quad isr_stub_16
    .quad isr_stub_17
    .quad isr_stub_18
    .quad isr_stub_19
    .quad isr_stub_20
    .quad isr_stub_21
    .quad isr_stub_22
    .quad isr_stub_23
    .quad isr_stub_24
    .quad isr_stub_25
    .quad isr_stub_26
    .quad isr_stub_27
    .quad isr_stub_28
    .quad isr_stub_29
    .quad isr_stub_30
    .quad isr_stub_31