#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/nmi.h>
#include <kernel/keyboard.h>
#include <kernel/page.h>

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

extern volatile uint32_t npages;                        /* npages表示可分配的页个数，因为链接后才能得到 _kernel_end 的值，所以无法在编译期间计算，要运行之后计算 */
extern volatile uint32_t *frame_map;                 /* frame_map标记某个页是否被使用，要放置在_kernel_end，同样也要运行之后计算 */
extern volatile uint32_t *startframe; 
void test_pagealloc() {
        pageframe_t page_array[200] = {0};
    unsigned int i = 0;

    int count = 0;
    for (i = 0; i < 200 ; ++i) {
        page_array[i] = kalloc_frame();
        if (!page_array[i]) {
            printf("No free pages left! %d\n", count);
            break;
        }
        count++;
    }

    printf("npages: %u\n", npages);
    printf("frame_map: %u\n", frame_map);
    printf("startframe: %u\n", startframe);

    kfree_frame(page_array[43]);
    kfree_frame(page_array[56]);
    kfree_frame(page_array[177]);
    count = 0;
    for (; i < 200; ++i) {
        page_array[i] = kalloc_frame();
        if (!page_array[i]) {
            printf("No free pages left! %d\n", count);
            break;
        }
        count++;
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
    // test_keyboard();
    test_pagealloc();


    __asm__ volatile ("hlt");
}