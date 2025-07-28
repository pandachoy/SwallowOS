.section .text
irq_stub_0:
    cli
    iret

irq_stub_1:
    call keyboard_handler
    iret

.section .data
.global irq_stub_table
irq_stub_table:
    .long irq_stub_0
    .long irq_stub_1