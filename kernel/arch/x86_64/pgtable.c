#include "constant.h"
#include "pgtable.h"

void *get_physaddr(uint64_t *pml4, void *virtualaddr) {
    uint64_t *pdptr = pml4[pml4_index(virtualaddr)] & ~0xFFF;
    uint64_t *pd = pdptr[pdptr_index(virtualaddr)] & ~0xFFF;
    uint64_t *pt = pd[pd_index(virtualaddr)] & ~0xFFF;
    return (void*)((uint64_t)(pt[pt_index(virtualaddr)] & ~0xFFFF000000000FFF) + ((uint64_t)virtualaddr & 0xFFF));
}
