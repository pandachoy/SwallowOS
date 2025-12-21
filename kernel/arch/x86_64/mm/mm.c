
#include <kernel/page.h>
#include <string.h>
#include "../include/defs.h"
#include "../cpu/cpu.h"
#include "mm.h"

#define TASK_STACK_PAGE_NUM     10

const uint64_t mm_rsp_offset = offset_of(struct mm_struct, rsp);
const uint64_t mm_rsp0_offset = offset_of(struct mm_struct, rsp0);
const uint64_t mm_tss_rsp0_offset = offset_of(struct mm_struct, tss_rsp0);
const uint64_t mm_pgd_offset = offset_of(struct mm_struct, pgd);

int mm_init(struct mm_struct *mm) {
    if (!mm) return -1;

    memset(mm, 0, sizeof(struct mm_struct));
    mm->stack0 = alloc_pages(TASK_STACK_PAGE_NUM);
    if (mm->stack0.page == 0) return -1;
    mm->rsp0 = mm->stack0.page + mm->stack0.npages * PAGE_SIZE;
    mm->tss_rsp0 = mm->rsp0;
    mm->stack = alloc_pages(TASK_STACK_PAGE_NUM);
    if (mm->stack.page == 0) {
        free_pages(&mm->stack0);
        return -1;
    }
    mm->rsp =  mm->stack.page + mm->stack.npages * PAGE_SIZE;
    return 0;
}

void mm_clean(struct mm_struct *mm) {
    free_pages(&mm->stack0);
    free_pages(&mm->stack);
}