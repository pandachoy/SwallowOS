#ifndef _MM_H
#define _MM_H
#include <stdint.h>
#include <kernel/list.h>
#include "pagemanager.h"

/* vm_flags value */
#define VM_READ           0x000001
#define VM_WRITE          0x000002
#define VM_EXEC           0x000003

struct mm_struct {
    // struct page_alloc stack;
    void* rsp;                              /* the task's kernel stack */
    uint64_t pgd;                           /* the task's virtual address space*/

    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, brk;
    unsigned long start_stack;

    char *elf_content;
    struct vm_area_struct *mmap;
};

struct vm_area_struct {
    uint64_t vm_start;
    uint64_t vm_end;
    uint64_t vm_flags;
    uint64_t vm_pgoff;
    struct mm_struct *vm_mm;
    struct list_head vma_list;
};

extern const uint64_t mm_rsp_offset;
extern const uint64_t mm_rsp0_offset;
extern const uint64_t mm_pgd_offset;
extern const uint64_t mm_state_offset;

int mm_init(struct mm_struct *mm);
void mm_clean(struct mm_struct *mm);
uint64_t mm_dup_pgd(int64_t src_pgd);

#endif