#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/nmi.h>
#include <kernel/keyboard.h>

extern volatile uint8_t key_buffer_pos;
extern volatile char keyboard_buffer[256];

void test_helloworld() {
    printf("Hello, %s!\nHi!", "World");
}

void test_keyboard() {
    while(1) {
        if (key_buffer_pos > 0) {
            // for (int i = 0; i < key_buffer_pos; i++) {
            //     printf(keyboard_buffer[i]);
            // }
            printf("%s", keyboard_buffer);
            key_buffer_pos = 0;
        }
    }

}

void kernel_main(void) {
    load_gdt();
    terminal_initialize();
    PIC_init();
    keyboard_init();
    load_idt();
    // NMI_enable();
    // NMI_disable();

    // test_helloworld();
    test_keyboard();


    __asm__ volatile ("hlt");

    // __asm__ volatile ("int $0x80");
}