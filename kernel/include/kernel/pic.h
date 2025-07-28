#ifndef _KERNEL_PIC_H
#define _KERNEL_PIC_H

void PIC_init();
void PIC_sendEOI(uint8_t irq);
void PIC_disable(void);
uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);

#endif