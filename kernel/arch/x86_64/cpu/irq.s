.include "arch/x86_64/cpu/cpu.inc"

.section .text
irq_stub_0:
    push_all
    callq timer_handler
    pop_all
    iretq

irq_stub_1:
    push_all
    callq keyboard_handler
    pop_all
    iretq

irq_stub_6:
    callq floppy_irq_handler
    iretq


.section .data
.global irq_stub_table
irq_stub_table:
    .quad irq_stub_0
    .quad irq_stub_1
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad irq_stub_6