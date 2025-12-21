#ifndef _PAGEMANAGER_H
#define _PAGEMANAGER_H

#include <stdint.h>
#include <stddef.h>
#include "../cpu/cpu.h"

/* 页帧格式 */
typedef uint64_t *pageframe_t;

struct page_alloc {
    pageframe_t page;
    uint64_t npages;
};

enum pf_error_code {
    PF_PROT       =         1 << 0,
    PF_WRITE      =         1 << 1,
    PF_USER       =         1 << 2,
    PF_RSVD       =         1 << 3,
    PF_INSTR      =         1 << 4,
    PF_PK         =         1 << 5
};

void kalloc_frame_init();
struct page_alloc alloc_pages(size_t count);
void free_pages(struct page_alloc *pa);
void page_fault_handler(unsigned long error_code);

#endif