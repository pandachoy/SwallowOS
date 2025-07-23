#include "../../libc/include/stdio.h"

#include <kernel/tty.h>
#include <kernel/gdt.h>

void kernel_main(void) {
    load_gdt();

    terminal_initialize();
    printf("Hello, %s!\nHi!", "World");
}