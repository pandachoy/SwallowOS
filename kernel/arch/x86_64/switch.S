.set RUNNING,    0x0
.set READY,      0x1    

# C declaration
# void switch_to_task(thread_control_block *next_thread);
# WARNING: Caller is expected to disable IRQs before calling
# and enable IRQs after the function returns

.global switch_to_task
.type switch_to_task, @function
switch_to_task:

    # save previous task's state
    # Notes:
    # For cdecl; rdi are already saved by the caller and don't need to be saved again
    # rip is already saved on the stack by the caller's "CALL" instruction
    # The task isn't able to change CR3 so it doesn't need to be saved
    # Segment registers are constants (while running kernel code) so they don't need to be saved
    push %rax
    push %rbx
    push %rcx
    push %rsi

    call update_time_used

    # save esp to current_task_TCB
    mov $current_task_TCB, %rsi
    mov (%rsi), %rsi
    mov $TCB_rsp_offset, %rcx
    mov (%rcx), %rcx
    add %rcx, %rsi
    mov %rsp, (%rsi)

    # set current task's state
    mov $current_task_TCB, %rsi
    mov (%rsi), %rsi
    mov $TCB_state_offset, %rcx
    mov (%rcx), %rcx
    add %rcx, %rsi
    mov $READY, %ecx
    mov %ecx, (%rsi)

    # load next task's state, next task saved in rdi
    mov %rdi, current_task_TCB
    # set next task's state
    mov $current_task_TCB, %rsi
    mov (%rsi), %rsi
    mov $TCB_state_offset, %rcx
    mov (%rcx), %rcx
    add %rcx, %rsi
    mov $RUNNING, %ecx
    mov %ecx, (%rsi)

    # load rsp
    mov $current_task_TCB, %rsi
    mov (%rsi), %rsi
    mov $TCB_rsp_offset, %rcx
    mov (%rcx), %rcx
    add %rcx, %rsi
    mov (%rsi), %rsp
    # load cr3
    mov $current_task_TCB, %rsi
    mov (%rsi), %rsi
    mov $TCB_cr3_offset, %rcx
    mov (%rcx), %rcx
    add %rcx, %rsi
    mov (%rsi), %rax
    # load rsp0
    mov $current_task_TCB, %rsi
    mov (%rsi), %rsi
    mov $TCB_rsp0_offset, %rcx
    mov (%rcx), %rcx
    add %rcx, %rsi
    mov (%rsi), %rbx

    # adjust the rsp0 field in the TSS
    mov $tss_entry, %rcx
    add $0x04, %rcx           # rsp0 offset in tss
    mov %rbx, (%rcx)

    # compare virtual address
    mov %cr3, %rcx
    cmpl %ecx, %eax
    je .doneVAS
    mov %rax, %cr3
.doneVAS:

    pop %rsi
    pop %rcx
    pop %rbx
    pop %rax
    ret

.global getcr3
.type getcr3, @function
getcr3:
    mov %cr3, %rax
    ret


    

    