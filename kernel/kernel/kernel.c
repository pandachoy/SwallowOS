#include "../../libc/include/stdio.h"

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

void kernel_main(void) {
    load_gdt();
    load_idt();

    terminal_initialize();
    printf("Hello, %s!\nHi!", "World");
}