#include "../include/constant.h"
#include "pgtable.h"

void virtaddr2page(const void *virtualaddr, unsigned int *pml4_idx, unsigned int *pdptr_idx, unsigned int *pd_idx, unsigned int *pt_idx) {
    *pml4_idx = pml4_index(virtualaddr);
    *pdptr_idx = pdptr_index(virtualaddr);
    *pd_idx = pd_index(virtualaddr);
    *pt_idx = pt_index(virtualaddr);
}

void *get_physaddr(const uint64_t *pml4, const void *virtualaddr) {
    if (virtualaddr > HIGHER_HALF_OFFSET)
        return virtualaddr - HIGHER_HALF_OFFSET;

    uint64_t *pdptr = pml4[pml4_index(virtualaddr)] & ~0xFFF;
    uint64_t *pd = pdptr[pdptr_index(virtualaddr)] & ~0xFFF;
    uint64_t *pt = pd[pd_index(virtualaddr)] & ~0xFFF;
    return (void*)((uint64_t)(pt[pt_index(virtualaddr)] & ~PAGE_ADDR_MASK) + ((uint64_t)virtualaddr & 0xFFF));
}