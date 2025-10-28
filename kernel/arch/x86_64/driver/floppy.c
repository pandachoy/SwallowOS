#include <stdio.h>
#include <stdbool.h>
#include <kernel/io.h>
#include <kernel/pic.h>
#include "floppy.h"

/* Floppy registers */
#define FDC_SRA      0x3F0                 /* status register B */
#define FDC_SRB      0x3F1                 /* status register A */
#define FDC_DOR      0x3F2                 /* Digital output */
#define FDC_MSR      0x3F4                 /* Main status */
#define FDC_DSR      0x3F4                 /* Datarate select */
#define FDC_DATA     0x3F5                 /* Data fifo */
#define FDC_DIR      0x3F7                 /* Digital input */
#define FDC_CCR      0x3F7                 /* Configuration control register */

#define FLOPPY_IRQ_NUMBER 0x06

static volatile bool irq6_fired = false;

static void motor_off(void);

void floppy_irq_handler() {
    irq6_fired = true;
    PIC_sendEOI(FLOPPY_IRQ_NUMBER);
}

static bool floppy_wait_irq_timeout(uint32_t spins) {
    while (spins--) {
        if (irq6_fired) {
            irq6_fired = false;
            return true;
        }
        io_wait();
    }

    return false;
}

static fdc_write(uint8_t val) {
    for(;;) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & 0xC0) == 0x80) break;        /* RQM=1, DIO=0 */
    }
    outb(FDC_DATA, val);
}

static uint8_t fdc_read(void) {
    for (;;) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & 0xC0) == 0xC0) break;        /* RQM=1, DIO=1 */
    }
    return inb(FDC_DATA);
}

static void fdc_send_cmd(uint8_t cmd) {
    fdc_write(cmd);
}

static void fdc_sense_interrupt(uint8_t *st0, uint8_t *cyl) {
    fdc_send_cmd(0x08);
    *st0 = fdc_read();
    *cyl = fdc_read();
}

static void fdc_reset(void) {
    /* reset controller */
    printk("fdc_reset\n");
    outb(FDC_DOR, 0x00);
    io_wait();
    outb(FDC_DOR, 0x0C);
    io_wait();
    for (int i = 0; i<4; ++i) {
        floppy_wait_irq_timeout(1e5);
        uint8_t st0, cyl;
        fdc_sense_interrupt(&st0, &cyl);
        printk("sense\n");
        (void)st0;
        (void)cyl;
    }
    outb(FDC_CCR, 0x00);       /* 500kpbs */
}

static bool fdc_calibrate(uint8_t drive) {
    for (int tries=0; tries < 10; ++ tries) {
        fdc_send_cmd(0x07);     /* calibrate */
        fdc_write(drive);
        if (!floppy_wait_irq_timeout(1e6)) {
            printk("calibrating timeout\n");
            continue;
        }
        uint8_t st0, cyl;
        fdc_sense_interrupt(&st0, &cyl);
        if (cyl == 0) return true;
    }
    return false;
}

static bool fdc_seek(uint8_t drive, uint8_t head, uint8_t cyl) {
    fdc_send_cmd(0x0F);
    fdc_write((head << 2) | drive);
    fdc_write(cyl);
    if (!floppy_wait_irq_timeout(2e6)) {
        printk("seeking timeout\n");
        motor_off();
        return false;
    }
    uint8_t st0, rcyl;
    fdc_sense_interrupt(&st0, &rcyl);
    return rcyl == cyl;
}

static uint8_t * const DMA_BUF = (uint8_t*)0x1000;

static void dma_setup_read(uint32_t addr, uint16_t length) {
    /* floppy disk controllers are hardwired to DMA channel 2*/
    /* 8237 DMA channel 2, single mode, read */
    outb(0x0A, 0x06);                            /* mask chan2 */
    outb(0x0C, 0xFF);                            /* reset flip-flop*/
    outb(0x04, addr & 0xFF);                      /* addr low */
    outb(0x04, (addr >> 8) & 0xFF);             /* addr high */
    outb(0x81, (addr >> 16) & 0xFF);             /* page for chan2*/
    outb(0x0C, 0xFF);
    uint16_t count = length - 1;
    outb(0x05, count & 0xFF);
    outb(0x05, (count >> 8) & 0xFF);
    outb(0x0B, 0x56);                            /* single, address increment, auto-init off, read, chan2 */
    outb(0x0A, 0x02);                            /* umask chan2 */
}

static void motor_on(uint8_t drive) {
    uint8_t dor = 0x1C | (drive & 0b11);     /* enable DMA/IRQ + motor */
    outb(FDC_DOR, dor);
    for (volatile int i = 0; i < 1e5; ++i) {
        __asm__ volatile ("nop");
    }
}

static void motor_off(void) {
    outb(FDC_DOR, 0x0C);
}

void floppy_init(void) {
    /* register irq6 */
    // idt_register_irq6(floppy_irq_handler); todo
    fdc_reset();
    printk("fdc_calibrate: %d\n",fdc_calibrate(0));
}

bool floppy_read_chs(uint8_t c, uint8_t h, uint8_t s, uint8_t *buffer512) {
    motor_on(0);
    if (!fdc_seek(0, h, c)) {
        motor_off();
        return false;
    }

    printk("dma setup\n");
    dma_setup_read(0x1000, 512);

    /* read data: MFM=1 SK=0 MT=0 */
    printk("read cmd\n");
    fdc_send_cmd(0xE6);
    fdc_write((h << 2) | 0);
    fdc_write(c);
    fdc_write(h);
    fdc_write(s);
    fdc_write(2);         /* 512B */
    fdc_write(18);        /* sectors per track */
    fdc_write(0x1B);      /* GAP3 length */
    fdc_write(0xFF);      /* DTL */

    if (!floppy_wait_irq_timeout(2e6)) {
        printk("read timeout\n");
        motor_off();
        return false;
    }

    /* read 7 result bytes */
    uint8_t st0 = fdc_read();
    uint8_t st1 = fdc_read();
    uint8_t st2 = fdc_read();
    uint8_t rc = fdc_read();
    uint8_t rh = fdc_read();
    uint8_t rs = fdc_read();
    uint8_t rsz = fdc_read();
    (void)st0; (void)st1; (void)st2; (void)rc; (void)rh; (void)rs; (void)rsz;

    for (int i = 0; i < 512; ++i)
        buffer512[i] = DMA_BUF[i];
    motor_off();
    return true;
}

static void lba_to_chs(uint32_t lba, uint8_t *c, uint8_t *h, uint8_t *s) {
    const uint32_t sectors_per_track = 18;
    const uint32_t headers_per_cylinder = 2;
    const uint32_t cylinders = 80;

    *c = lba / (headers_per_cylinder * sectors_per_track);
    *h = (lba % (headers_per_cylinder * sectors_per_track)) / sectors_per_track;
    *s = (lba % (headers_per_cylinder * sectors_per_track)) % sectors_per_track + 1;
}

static void dma_setup_write(uint32_t addr, uint16_t length) {
    outb(0x0A, 0x06);                 /* mask chan2 */
    outb(0x0C, 0xFF);                 /* reset flip-flop */
    outb(0x04, addr & 0xFF);
    outb(0x04, (addr >> 8) & 0xFF);
    outb(0x81, (addr >> 16) & 0xFF);
    outb(0x0C, 0xFF);
    uint16_t count = length - 1;
    outb(0x05, count & 0xFF);
    outb(0x05, (count >> 8) & 0xFF);
    outb(0x0B, 0x5A);                  /* single, address increment, write, chan2 (mem->IO) */
    outb(0x0A, 0x02);                  /* unmask chan2 */
}

bool floppy_read_lba(uint32_t lba, uint8_t *buffer512) {
    uint8_t c, h, s;
    lba_to_chs(lba, &c, &h, &s);
    return floppy_read_chs(c, h, s, buffer512);
}

bool floppy_write_lba(uint32_t lba, const uint8_t *buffer512) {
    motor_on(0);
    uint8_t c, h, s;

    lba_to_chs(lba, &c, &h, &s);
    if (!fdc_seek(0, h, c)) {
        motor_off();
        return false;
    }

    /* copy data to DMA buffer */
    for (int i = 0; i < 512; ++i)
        DMA_BUF[i] = buffer512[i];
    dma_setup_write(0x1000, 512);

    /* write data mfm */
    printk("write cmd\n");
    fdc_send_cmd(0xC5);
    fdc_write((h << 5) | 0);
    fdc_write(c);
    fdc_write(h);
    fdc_write(s);
    fdc_write(2);
    fdc_write(18);
    fdc_write(0x1B);
    fdc_write(0xFF);

    if (!floppy_wait_irq_timeout(1e6)) {
        printk("write timeout\n");
        motor_off();
        return false;
    }

    /* result bytes */
    (void)fdc_read(); (void)fdc_read(); (void)fdc_read();
    (void)fdc_read(); (void)fdc_read(); (void)fdc_read(); (void)fdc_read();
    motor_off();
    return true;
}