#ifndef FLOPPY_H
#define FLOPPY_H

#include <stdbool.h>

void floppy_init(void);
bool floppy_read_chs(uint8_t cylinder, uint8_t head, uint8_t sector, uint8_t *buffer512);
bool floppy_read_lba(uint32_t lba, uint8_t *buffer512);
bool floppy_write_lba(uint32_t lba, const uint8_t *buffer512);
void floppy_irq_handler();

#endif