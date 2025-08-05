#include <kernel/io.h>
#include <kernel/pic.h>

#define PIC1		    0x20		/* IO base address for master PIC */
#define PIC2		    0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	    (PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	    (PIC2+1)

#define PIC_EOI          0x20     /* End-of-interrupt, 当一个interrupt结束时发送这个command*/

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */

#define ICW1_ICW4	     0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	     0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	 0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	     0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	     0x10		/* Initialization - required! */

#define ICW4_8086    	 0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	     0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	 0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	 0x0C		/* Buffered mode/master */
#define ICW4_SFNM	     0x10		/* Special fully nested (not) */

/* for innterrupt status registers reading */
#define PIC_READ_IRR     0x0a     /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR     0x0b     /* OCW3 irq service next CMD read */  


/* 向8259发送EOI */
void PIC_sendEOI(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);    /* 如果irq大于8，说明是第二个芯片，要给两个芯片都发送EOI */

    outb(PIC1_COMMAND, PIC_EOI);
}

/*
arguments:
	offset1 - vector offset for master PIC
		vectors on the master become offset1..offset1+7
	offset2 - same for slave PIC: offset2..offset2+7
*/
void PIC_remap(int offset1, int offset2) {
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();
	
	outb(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	// Unmask both PICs.
	outb(PIC1_DATA, 0);
	outb(PIC2_DATA, 0);
}

/* in protected mode, set the master PIC's offset to 0x20 and the slave's to 0x28 */
void PIC_init() {
    PIC_remap(0x20, 0x28);
}

void PIC_disable(void) {
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

void IRQ_set_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if (IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);
}

void IRQ_clear_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if (IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);
}

static uint16_t __pic_get_irq_reg(int ocw3) {
    /* OCW3 to PIC CMD to get the register values. */
    /* PIC2 is chained, and represents IRQs 8-15. */
    /* PIC1 is IRQs 0-7, with 2 being the chain */
    outb(PIC1_COMMAND, ocw3);
    outb(PIC2_COMMAND, ocw3);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

// 定义一个函数，用于获取中断请求寄存器（IRR）的值
// 返回类型为 uint16_t，表示返回一个16位无符号整数
uint16_t pic_get_irr(void) {
    // 调用内联汇编函数 __pic_get_irq_reg，并传递参数 PIC_READ_IRR
    // __pic_get_irq_reg 是一个内联汇编函数，用于读取8259 PIC（可编程中断控制器）的寄存器值
    // PIC_READ_IRR 是一个宏或常量，用于指定读取IRR（中断请求寄存器）
    return __pic_get_irq_reg(PIC_READ_IRR);
}

uint16_t pic_get_isr(void) {
    return __pic_get_irq_reg(PIC_READ_ISR);
}
