#include <stdbool.h>

#include <kernel/idt.h>
#include <kernel/tty.h>
#include <kernel/keyboard.h>
#include <kernel/printk.h>
#include "cpu.h"

#define IDT_MAX_DESCRIPTORS 			256
#define IDT_CPU_EXCEPTION_COUNT			32
#define IDT_IRQ_COUNT                   7

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

typedef struct {
    uint16_t base_low;        /* The lower 16 bits of the ISR's address*/
    uint16_t kernel_cs;      /* The GDT segment selector that the CPU will load into CS before calling the ISR */
    uint8_t ist;              /* The IST in the TSS that the CPU will load into RSP; set to zero for now */
    uint8_t attributes;       /* Type and attributes */
    uint16_t base_mid;        /* The higher 16 bits of the lower 32 bits */
    uint32_t base_high;       /* The higher 32 bits of the ISR's address */
    uint32_t reserved;        /* Set to zero */
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

__attribute__((aligned(0x10)))
static idt_entry_t idt[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

struct exc_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

__attribute__((noreturn))
void exception_handler(uint64_t vector, uint64_t error_code, struct exc_frame *frame) {
    cli();
    uint64_t cr2 = getcr2();
    uint64_t cur_rsp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(cur_rsp));
    printk("[kernel] exception! vec=%u err=%x cr2=%x rip=%x cs=%x rflags=%x rsp=%x\n",
           (unsigned)vector, (unsigned)error_code, (unsigned)cr2,
           (unsigned)frame->rip, (unsigned)frame->cs,
           (unsigned)frame->rflags, (unsigned)frame->rsp);
    /* dump 20 qwords starting well below the exception frame to see
       what was on the task kernel stack before the fault */
    uint64_t *p = (uint64_t*)(cur_rsp + 24);  /* skip stub pushes + call ret */
    printk("stack dump (from exc handler rsp):\n");
    for (int i = 0; i < 24; ++i) {
        uint64_t val = p[i];
        printk("  [%x] %x\n", (unsigned)(uintptr_t)&p[i], (unsigned)val);
        if (val == 0 && i > 16) break;
    }
    hlt();
    sti();
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags) {
    idt_entry_t *descriptor = &idt[vector];

    descriptor->base_low      = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs     = 0x08;
    descriptor->ist           = 0;
    descriptor->attributes    = flags;
    descriptor->base_mid      = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->base_high     = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
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

    // __asm__ volatile ("cli"); /* unset the interrupt flag */
    __asm__ volatile ("lidt %0" : : "m"(idtr)); /* load the new IDT */
    // __asm__ volatile ("sti"); /* set the interrupt flag */
    // sti();
}

void cli() {
    __asm__ volatile ("cli");
}

void sti() {
    __asm__ volatile ("sti");
}