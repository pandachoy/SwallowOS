#ifndef _PGTABLE_H
#define _PGTABLE_H

#include <stdint.h>

#define pml4_index(address)         (((unsigned long)address >> 39) & 0x1ff)
#define pdptr_index(address)        (((unsigned long)address >> 30) & 0x1ff)
#define pd_index(address)           (((unsigned long)address >> 21) & 0x1ff)
#define pt_index(address)           (((unsigned long)address >> 12) & 0x1ff)

#define PAGE_ADDR_MASK                                    0xFFFF000000000FFF
#define ENTRY_NUM                                                        512


void virtaddr2page(const void *virtualaddr, unsigned int *pml4_idx, unsigned int *pdptr_idx, unsigned int *pd_idx, unsigned int *pt_idx);
void *get_physaddr(const uint64_t *pml4, const void *virtualaddr);


#endif