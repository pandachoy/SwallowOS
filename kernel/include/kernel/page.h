#ifndef _PAGE_H
#define _PAGE_H

#define PAGE_SHIFT                            12
#define PAGE_SIZE              (1 << PAGE_SHIFT)

void init_page() __attribute__((section(".multiboot.text")));
void load_page_directory() __attribute__((section(".multiboot.text")));
void enable_paging() __attribute__((section(".multiboot.text")));

#endif