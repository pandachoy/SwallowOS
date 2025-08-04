#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H

#include <stdint.h>

void set_gdt_entry(uint32_t *entry, uint32_t base, uint32_t limit, uint16_t flags) __attribute__((optimize("O0")));
void load_gdt();

#endif