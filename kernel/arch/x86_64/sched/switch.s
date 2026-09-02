.set RUNNING,    0x0
.set READY,      0x1   
.include "arch/x86_64/cpu/cpu.inc"

# C declaration
# void switch_to_task(thread_control_block *next_thread);
# WARNING: Caller is expected to disable IRQs before calling
# and enable IRQs after the function returns

.global switch_to_task
.type switch_to_task, @function
switch_to_task:

    pushfq
    cli

    call update_time_used

    # load current_task_TCB
    mov current_task_TCB(%rip), %rsi

    # save rsp to current_task_TCB
    mov TCB_rsp0_offset(%rip), %rcx
    mov %rsp, (%rsi, %rcx, 1)
    # load next task's state, next task saved in rdi
    mov %rdi, current_task_TCB

    # set next task's state
    mov current_task_TCB(%rip), %rsi
    mov TCB_state_offset(%rip), %rcx
    mov $RUNNING, (%rsi, %rcx, 1)
    
    # load rsp0
    mov TCB_rsp0_offset(%rip), %rcx
    mov (%rsi, %rcx, 1), %rsp


    # load mm->pgd
    mov TCB_mm_offset(%rip), %rcx
    mov (%rsi, %rcx, 1), %rsi           # load mm into %rsi
    cmp $0, %rsi
    je .restore_kernel_pgd
    # read pgd
    mov mm_pgd_offset(%rip), %rcx
    mov (%rsi, %rcx, 1), %rax
    mov %rax, %cr3
    jmp .doneVAS
.restore_kernel_pgd:
    mov kernel_pgd(%rip), %rax
    mov %rax, %cr3
.doneVAS:


    popf
    ret



    

    