#ifndef _PGTABLE_H
#define _PGTABLE_H

#include <stdint.h>

#define pml4_index(address)         (((unsigned long)address >> 39) & 0x1ff)
#define pdptr_index(address)        (((unsigned long)address >> 30) & 0x1ff)
#define pd_index(address)           (((unsigned long)address >> 21) & 0x1ff)
#define pt_index(address)           (((unsigned long)address >> 12) & 0x1ff)

/* mpl4 */
extern void *page_map_level4;
// /* pdptr */
// extern void *first_page_directory_ptr; /* identity mapping pdtr */
// extern void *last_page_directory_ptr;
// /* pd */
// extern void *last_first_page_directory;
// extern void *last_second_page_directory;
// /* pt */
// extern void *first_page_table;
// extern void *second_page_table;

void *get_physaddr(uint64_t *pml4, void *virtualaddr);


#endif