#include "../multiboot/multiboot.h"
#include "ram.h"
#include "constant.h"

extern void *p_multiboot_info;
uint64_t ram_start = 0;
uint64_t ram_end = 0;
void init_ram() {
    multiboot_info_t *mbd = p_multiboot_info;

    /* collect info of available memory */
    if(!(mbd->flags >> 6 & 0x1)) {
        printf("invalid memory map");
        return;
    }

    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t *mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);
        printf("Start Addr: %u | Length: %u | Size: %u | Type: %d\n",
            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
        
        /* choose availabel range */
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            printf("This mem map is available\n");
            printf("init_ram: %u\n", init_ram);

            /* not accuraccy check */
            if ((void*)init_ram - HIGHER_HALF_OFFSET > mmmt->addr && (void*)init_ram - HIGHER_HALF_OFFSET  < mmmt->addr + mmmt->len) {
                ram_start = mmmt->addr;
                ram_end = mmmt->addr + mmmt->len;
                break;
            }
        }
    }
}