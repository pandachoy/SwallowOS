.section .text
irq_stub_0:
    callq timer_handler
    iretq

irq_stub_1:
    callq keyboard_handler
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