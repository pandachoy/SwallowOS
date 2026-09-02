#include <stdint.h>
#include <kernel/pic.h>
#include <kernel/io.h>
#include "../sched/task.h"

#define PIT_IRQ_NUMBER        0x0
#define PIT_IRQ_VECTOR       0x20

volatile unsigned long timer_count = 0;

void timer_handler(void) {
    PIC_sendEOI(PIT_IRQ_NUMBER);
    timer_count++;
    task_hook_in_timer_handler();
}

void timer_init(void) {
    unsigned long divisor = 1193180 / 1000;
    unsigned char l = (unsigned char)(divisor & 0xff);
    unsigned char h = (unsigned char)((divisor >> 8) & 0xff);

    /* init pit, ref: https://wiki.osdev.org/Programmable_Interval_Timer*/
    outb(0x43, 0x36);    /* rate generator; libyte/hibyte; channel 0 */
    outb(0x40, l);
    outb(0x40, h);
}

unsigned long get_timer_count() {
    return timer_count;
}

