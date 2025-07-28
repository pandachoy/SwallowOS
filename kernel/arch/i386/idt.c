#include <stdbool.h>

#include <kernel/idt.h>
#include <kernel/tty.h>
#include <kernel/keyboard.h>

#define IDT_MAX_DESCRIPTORS 			256
#define IDT_CPU_EXCEPTION_COUNT			32
#define IDT_IRQ_COUNT                   2

#define IDT_DESCRIPTOR_X16_INTERRUPT	0x06
#define IDT_DESCRIPTOR_X16_TRAP 		0x07
#define IDT_DESCRIPTOR_X32_TASK 		0x05
#define IDT_DESCRIPTOR_X32_INTERRUPT  	0x0E
#define IDT_DESCRIPTOR_X32_TRAP			0x0F
#define IDT_DESCRIPTOR_RING1  			0x40
#define IDT_DESCRIPTOR_RING2  			0x20
#define IDT_DESCRIPTOR_RING3  			0x60
#define IDT_DESCRIPTOR_PRESENT			0x80

#define IDT_DESCRIPTOR_EXCEPTION		(IDT_DESCRIPTOR_X32_INTERRUPT | IDT_DESCRIPTOR_PRESENT)
#define IDT_DESCRIPTOR_EXTERNAL			(IDT_DESCRIPTOR_X32_INTERRUPT | IDT_DESCRIPTOR_PRESENT)
#define IDT_DESCRIPTOR_CALL				(IDT_DESCRIPTOR_X32_INTERRUPT | IDT_DESCRIPTOR_PRESENT | IDT_DESCRIPTOR_RING3)

__attribute__((aligned(0x10)))
static idt_entry_t idt[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

__attribute__((noreturn))
void exception_handler(void) {
    __asm__ volatile ("cli");
    __asm__ volatile ("sti");
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    idt_entry_t *descriptor = &idt[vector];

    descriptor->base_low       = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs     = 0x08; /* kernel code in GDT */
    descriptor->attributes    = flags;
    descriptor->base_high      = (uint32_t)isr >> 16;
    descriptor->reserved      = 0;
}

static bool vectors[IDT_MAX_DESCRIPTORS];

extern void* isr_stub_table[];
extern void* irq_stub_table[];

void load_idt() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;
    
    /* isr */
    for (uint8_t vector = 0; vector < IDT_CPU_EXCEPTION_COUNT; ++vector) {
        idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXCEPTION);
        vectors[vector]=true;
    }
    /* irq */
    for (uint8_t vector = IDT_CPU_EXCEPTION_COUNT; vector < IDT_CPU_EXCEPTION_COUNT + IDT_IRQ_COUNT; ++vector) {
        idt_set_descriptor(vector, irq_stub_table[vector - IDT_CPU_EXCEPTION_COUNT], IDT_DESCRIPTOR_EXTERNAL);
        vectors[vector]=true;
    }

    __asm__ volatile ("cli"); /* unset the interrupt flag */
    __asm__ volatile ("lidt %0" : : "m"(idtr)); /* load the new IDT */
    __asm__ volatile ("sti"); /* set the interrupt flag */
}