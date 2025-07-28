#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

#include <stdint.h>

typedef struct {
    uint16_t base_low;       /* base的低16位 */
    uint16_t kernel_cs;     /* 在调用base时，需要加载的cs寄存器 */
    uint8_t reserved;       /* 设置为0 */
    uint8_t attributes;     /* 类型、属性等*/
    uint16_t base_high;      /* base的高16位 */
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
void load_idt();


#endif