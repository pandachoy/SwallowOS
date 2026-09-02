
#include <kernel/page.h>
#include <string.h>
#include "../include/defs.h"
#include "../include/constant.h"
#include "../cpu/cpu.h"
#include "mm.h"
#include "pagemanager.h"
#include "pgtable.h"

#define USER_TASK_STACK_PAGE_NUM     10

const uint64_t mm_rsp_offset = offset_of(struct mm_struct, rsp);
const uint64_t mm_pgd_offset = offset_of(struct mm_struct, pgd);

int mm_init(struct mm_struct *mm) {
    if (!mm) return -1;

    memset(mm, 0, sizeof(struct mm_struct));
    // mm->stack = alloc_pages(USER_TASK_STACK_PAGE_NUM);
    // if (mm->stack.page == 0) {
    //     return -1;
    // }
        /* free vma */

    return 0;
}

static int is_huge_page(uint64_t entry) {
    // return (entry & 0b10000000 > 0) ? 0 : -1;
        return ((entry & 0b10000000) > 0) ? 0 : -1;
}

void mm_clean_pgd(uint64_t pgd) {
    uint64_t *pgd_addr = pgd + HIGHER_HALF_OFFSET;

    for (unsigned int pml4_index = 0; pml4_index < ENTRY_NUM; ++pml4_index) {
        if (pml4_index >= 256) continue;
        if (pgd_addr[pml4_index] == 0) continue;
        uint64_t *pdptr_addr = (pgd_addr[pml4_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
        if (is_huge_page(pgd_addr[pml4_index]) != 0) {
            for (unsigned int pdptr_index = 0; pdptr_index < ENTRY_NUM; ++pdptr_index) {
                if (pdptr_addr[pdptr_index] == 0) continue;
                /* skip PS=1 entries, they're not pointers to sub-tables */
                if (is_huge_page(pdptr_addr[pdptr_index]) == 0) continue;
                uint64_t *pd_addr = (pdptr_addr[pdptr_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
                if (is_huge_page(pdptr_addr[pdptr_index]) != 0) {
                    for (unsigned int pd_index = 0; pd_index < ENTRY_NUM; ++pd_index) {
                        if (pd_addr[pd_index] == 0) continue;
                        uint64_t *pt_addr = (pd_addr[pd_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
                        if (is_huge_page(pd_addr[pd_index]) != 0) {
                            for (unsigned int pt_index = 0; pt_index < ENTRY_NUM; ++pt_index) {
                                if (pt_addr[pt_index] == 0) continue;
                                uint64_t *page_addr = (pt_addr[pt_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
                                struct page_alloc pa = {page_addr, 1};
                                free_pages(&pa);
                            }
                            // struct page_alloc pa = {pt_addr, 1};
                            // free_pages(&pa);
                        }
                        struct page_alloc pa = {pt_addr, 1};
                        free_pages(&pa);
                    }
                }
                struct page_alloc pa = {pd_addr, 1};
                free_pages(&pa);
            }
        }
        /* free pdptr */
        struct page_alloc pa = {pdptr_addr, 1};
        free_pages(&pa);
    }
}

uint64_t mm_dup_pgd(int64_t src_pgd) {
    uint64_t *src_pgd_addr = src_pgd + HIGHER_HALF_OFFSET, *dst_pgd_addr = NULL;
    uint64_t *src_pdptr_addr, *dst_pdptr_addr, *src_pd_addr, *dst_pd_addr, *src_pt_addr, *dst_pt_addr, *src_page_addr, *dst_page_addr;
    struct page_alloc pa = alloc_pages(1);
    uint64_t dst_pgd;
    int dup_res = 0;

    if (pa.npages == 0)
        return 0;
    dst_pgd_addr = pa.page;
    dst_pgd = ((uint64_t)dst_pgd_addr - HIGHER_HALF_OFFSET);

    for (unsigned int pml4_index = 0; pml4_index < ENTRY_NUM; ++pml4_index) {
        /* alloc pdptr */
        if (src_pgd_addr[pml4_index] == 0) continue;
        struct page_alloc pa = alloc_pages(1);
        if (pa.npages == 0) {
            printk("[Error] Failed to alloc page for pdptr: index (%u)\n", pml4_index);
            dup_res = -1; goto out;
        }
        dst_pdptr_addr = pa.page;
        dst_pgd_addr[pml4_index] = (src_pgd_addr[pml4_index] & 0b111) + ((uint64_t)dst_pdptr_addr - HIGHER_HALF_OFFSET);
        src_pdptr_addr = ((uint64_t)src_pgd_addr[pml4_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
        memcpy(dst_pdptr_addr, src_pdptr_addr, PAGE_SIZE);
        if (is_huge_page(src_pgd_addr[pml4_index]) == 0) continue;
        for (unsigned int pdptr_index = 0; pdptr_index < ENTRY_NUM; ++pdptr_index) {
            if (src_pdptr_addr[pdptr_index] == 0) continue;
            /* source PDP entry has PS=1: preserve huge page, skip PD allocation */
            if (is_huge_page(src_pdptr_addr[pdptr_index]) == 0) {
                    dst_pdptr_addr[pdptr_index] = src_pdptr_addr[pdptr_index];
                    continue;
            }
            struct page_alloc pa = alloc_pages(1);
            if (pa.npages == 0) {
                printk("[Error] Failed to alloc page for pd: index (%u %u)\n", pml4_index, pdptr_index);
                dup_res = -1; goto out;
            }
            dst_pd_addr = pa.page;
            dst_pdptr_addr[pdptr_index] = (src_pdptr_addr[pdptr_index] & 0b111) + ((uint64_t)dst_pd_addr - HIGHER_HALF_OFFSET);
            src_pd_addr = ((uint64_t)src_pdptr_addr[pdptr_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
            memcpy(dst_pd_addr, src_pd_addr, PAGE_SIZE);
            for (unsigned int pd_index = 0; pd_index < ENTRY_NUM; ++pd_index) {
                if (src_pd_addr[pd_index] == 0) continue;
                /* source PD entry has PS=1: preserve huge page, skip PT allocation */
                if (is_huge_page(src_pd_addr[pd_index]) == 0) {
                    dst_pd_addr[pd_index] = src_pd_addr[pd_index];
                    continue;
                }
                struct page_alloc pa = alloc_pages(1);
                if (pa.npages == 0) {
                    printk("[Error] Failed to alloc page for pt: index (%u %u %u)\n", pml4_index, pdptr_index, pd_index);
                    dup_res = -1; goto out;
                }
                dst_pt_addr = pa.page;
                dst_pd_addr[pd_index] = (src_pd_addr[pd_index] & 0b111) + ((uint64_t)dst_pt_addr - HIGHER_HALF_OFFSET);
                src_pt_addr = ((uint64_t)src_pd_addr[pd_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
                memcpy(dst_pt_addr, src_pt_addr, PAGE_SIZE);
                for (unsigned int pt_index = 0; pt_index < ENTRY_NUM; ++pt_index) {
                    if (src_pt_addr[pt_index] == 0) continue;
                    struct page_alloc pa = alloc_pages(1);
                    if (pa.npages == 0) {
                        printk("[Error] Failed to alloc page for page: index (%u %u %u %u)\n", pml4_index, pdptr_index, pd_index, pt_index);
                        dup_res = -1; goto out;
                    }
                    dst_page_addr = pa.page;
                    dst_pt_addr[pt_index] = (src_pt_addr[pt_index] & 0b111) + ((uint64_t)dst_page_addr - HIGHER_HALF_OFFSET);
                    src_page_addr = ((uint64_t)src_pt_addr[pt_index] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
                    memcpy(dst_page_addr, src_page_addr, PAGE_SIZE);
                }
            }
        }
    }
    dup_res = 0;

out:
    if (dup_res != 0) {
        mm_clean_pgd(dst_pgd);
        struct page_alloc pa = {dst_pgd_addr, 1};
        free_pages(&pa);
        return 0;
    }
    return dst_pgd;
}

void mm_clean(struct mm_struct *mm) {
    // free_pages(&mm->stack);

    /* clean vma */
    if (mm->mmap) {
        struct list_head *p = &mm->mmap->vma_list;

        while (p != p->prev) {
            struct vm_area_struct *vma = container_of(p, struct vm_area_struct, vma_list);
            list_del(p->prev);
            kfree(vma);
        }
        struct vm_area_struct *vma = container_of(p, struct vm_area_struct, vma_list);
        kfree(vma);
    }

    /* free pgd */
    uint64_t *pgd_addr = (mm->pgd & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET;
    struct page_alloc pa = {(void*)pgd_addr, 1};
    free_pages(&pa);

    /* free file_content */
    if (mm->elf_content)
        kfree(mm->elf_content); 
}