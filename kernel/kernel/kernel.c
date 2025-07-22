#include "../../libc/include/stdio.h"

#include <kernel/tty.h>

void kernel_main(void) {
    terminal_initialize();
    printf("Hello, %s!\nHi!", "World");
}