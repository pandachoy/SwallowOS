.section .data
gdtr:
    .word # limit
    .long # base

.section .text

.global set_gdt
.type set_gdt, @function
set_gdt:
    movl 4(%esp), %eax
    movw %ax, gdtr
    movl 8(%esp), %eax
    movl %eax, gdtr + 2


    lgdt gdtr

    cli

    in $0x92, %al
    orb $0x02, %al
    out %al, $0x92


    movl %cr0, %eax
    orb  $0x01, %al
    movl %eax, %cr0

    # reload cs register containing code selector
    ljmp $(0x08),$.reload_cs

.reload_cs:
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    movl %eax, %ss
    ret
