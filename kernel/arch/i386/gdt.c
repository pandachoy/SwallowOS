
#include <kernel/gdt.h>

static uint32_t gdt_entries[GDT_COUNT][2] = {0};

extern void set_gdt(unsigned long limit, uint32_t *base);

void set_gdt_entry(uint32_t *entry, uint32_t base, uint32_t limit, uint16_t flags){

    *entry =  base          <<  16;
    *entry |= limit         &   0x0000FFFF;

    entry++;

    *entry =  limit         &   0x000F0000;
    *entry |= (flags << 8)  &   0x00F0FF00;
    *entry |= (base >> 16)  &   0x000000FF;
    *entry |= base          &   0xFF000000;

}

void load_gdt(){
    /* kernel */
    set_gdt_entry(gdt_entries[1], 0, 0xFFFFFFFF, GDT_CODE_PL0);
    set_gdt_entry(gdt_entries[2], 0, 0xFFFFFFFF, GDT_DATA_PL0);
    /* video */
    set_gdt_entry(gdt_entries[3], 0, 0xFFFFFFFF, GDT_VIDEO);
    /* user */
    set_gdt_entry(gdt_entries[4], 0, 0xFFFFFFFF, GDT_CODE_PL3);
    set_gdt_entry(gdt_entries[5], 0, 0xFFFFFFFF, GDT_DATA_PL3);

    set_gdt(GDT_COUNT * GDT_ENTRY_SIZE, gdt_entries);
}