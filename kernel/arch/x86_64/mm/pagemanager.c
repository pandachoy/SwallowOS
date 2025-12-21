
#include <stddef.h>
#include "../include/constant.h"
#include "ram.h"
#include "pagemanager.h"
#include "pgtable.h"
#include <kernel/page.h>
#include <kernel/printk.h>
#include <string.h>
#include "../sched/task.h"


/* 页分配有很多方法，如bitmap、stack/list、buddy alocations等，这里用最简单的bitmap */
#define MEM_END       (HIGHER_HALF_OFFSET + 512 * 4096)               /* 暂时只讨论当前映射的一个页目录项 */
#define UINT64_BITS                           64
#define PRE_ALLOCATING_NUM                    20

/* 用来计算内核代码之后的 address of first page frame */
extern uint64_t _kernel_end;



uint64_t npages = 0;                        /* npages表示可分配的页个数，因为链接后才能得到 _kernel_end 的值，所以无法在编译期间计算，要运行之后计算 */
uint64_t *frame_map = NULL;                 /* frame_map标记某个页是否被使用，要放置在_kernel_end，同样也要运行之后计算 */
uint64_t *startframe = NULL;                /* 页帧起点，运行之后确定值 */
uint64_t *endframe = NULL;

/* frame map operation */
static uint64_t get_frame_map(unsigned int index) {
    return (frame_map[index / UINT64_BITS] & (1 << (index % UINT64_BITS)))== 0 ? 0 : 1;
}
static void set_frame_map(unsigned int index, uint64_t val) {
    if (val > 0)
        frame_map[index/UINT64_BITS] |= 1 << (index % UINT64_BITS);
    else
        frame_map[index/UINT64_BITS] &= ~(1 << (index % UINT64_BITS));
}


extern void *gdt_ptr;
static void remap_lower_kernel_range() {
    printk("_kernel_end: %x\n",  &_kernel_end);

    /* reset gdt */
    // char *gdt_ptr_addr = &gdt_ptr;
    // *(long*)(gdt_ptr_addr + 2) = *(long*)(gdt_ptr_addr + 2) + HIGHER_HALF_OFFSET;
    // __asm__ volatile("lgdt %0" :: "m"(gdt_ptr_addr));

    uint64_t *page_map_level4 = getcr3() + HIGHER_HALF_OFFSET;

    /* assume kernel less than 1G */
    uint64_t kernel_end_addr = &_kernel_end;
    if (kernel_end_addr < HIGHER_HALF_OFFSET)
        panic("_kernel_end less than %x: %x\n", HIGHER_HALF_OFFSET, kernel_end_addr);
    if (kernel_end_addr - HIGHER_HALF_OFFSET > 1024 * 1024 * 1024)
        panic("kernel range too big: %x\n", kernel_end_addr);

    
    /* remap lower kernel range in page granularity */
    /* 1. alloc kernel_page_directory_ptr */
    struct page_alloc pa = alloc_pages(1);
    if (pa.npages == 0)
        panic("Failed to alloc page of kernel_page_directory_ptr\n");
    uint64_t *kernel_page_directory_ptr = pa.page;
    memset(kernel_page_directory_ptr, 0, PAGE_SIZE);

    uint64_t *kernel_page_dir = NULL, *kernel_page_table = NULL;
    uint64_t kernel_lower_mapped = 0;
    for (unsigned int pdtr_index = 0; pdtr_index < ENTRY_NUM; ++pdtr_index) {
        /* 2. alloc kernel_page_dir */
        pa = alloc_pages(1);
        if (pa.npages == 0 && pa.page < HIGHER_HALF_OFFSET)
            panic("Failed to alloc page of kernel_page_dir, index: (%u)\n", pdtr_index);
        kernel_page_dir = pa.page;
        kernel_page_directory_ptr[pdtr_index] = (uint64_t)(kernel_page_dir) - HIGHER_HALF_OFFSET + 0x07;
        memset(kernel_page_dir, 0, PAGE_SIZE);
        for (unsigned int pd_index = 0; pd_index < ENTRY_NUM; ++pd_index) {
            /* 3. alloc kernel_page_table */
            pa = alloc_pages(1);
            if (pa.npages == 0 && pa.page < HIGHER_HALF_OFFSET)
                panic("Failed to alloc page of kernel_page_table, index: (%u, %u)\n", pdtr_index, pd_index);
            kernel_page_table = pa.page;
            kernel_page_dir[pd_index] = (uint64_t)(kernel_page_table) - HIGHER_HALF_OFFSET + 0x07;
            memset(kernel_page_table, 0, PAGE_SIZE);
            for (unsigned int pt_index = 0; pt_index < ENTRY_NUM; ++pt_index) {
                /* map kernel page */
                kernel_lower_mapped = (((pdtr_index * ENTRY_NUM) + pd_index) * ENTRY_NUM + pt_index) * PAGE_SIZE;
                if (kernel_lower_mapped > kernel_end_addr - HIGHER_HALF_OFFSET)
                    goto mapping_end;
                kernel_page_table[pt_index] = kernel_lower_mapped + 0x03;        /* in page table u/s must not be set! */
            }
        }
    }
    mapping_end:
        page_map_level4[0] = (uint64_t)kernel_page_directory_ptr - HIGHER_HALF_OFFSET + 0x07;
        setcr3((uint64_t)page_map_level4 - HIGHER_HALF_OFFSET);
}

// 函数 kalloc_frame_init 用于分配并初始化一个页面帧
void kalloc_frame_init() {
    /* init ram */
    init_ram();

    /* frame_map要sizeof(page_status)对齐 */
    if (!frame_map) {
        frame_map = (uint64_t *)(&_kernel_end + 1);
        /* 在frame_map后面填充其数据以及确定startframe */
        /* 首先要选一个合适的npages，设置为一个接近值x，则(x*4096 + x / 64) < (ram_end -  frame_map)， 解出 x = (ram_end - frame_map) / (4096 + 1/64), 实际上除数可以算成4097来估算 */
        npages = (ram_end + HIGHER_HALF_OFFSET - (uint64_t)frame_map) / (PAGE_SIZE + 1);
        /* 得到npages，即可容易算出startframe，注意4k对齐 */
        startframe = frame_map + (npages / UINT64_BITS);
        if ((uint64_t)startframe % PAGE_SIZE != 0)
            startframe = (uint64_t*)(((uint64_t)startframe / PAGE_SIZE + 1) * PAGE_SIZE);
        /* 检查页面是否超出内存，注意这里要考虑到VGA video占用的页，所以在比较的时候要减去4096 */
        while (startframe + PAGE_SIZE * npages > ram_end + HIGHER_HALF_OFFSET - PAGE_SIZE)
            npages--;
        for (uint64_t *p = frame_map; p < startframe; ++p)
            *p = 0;
    }

    remap_lower_kernel_range();
}

static int check_continuous_free_page(uint64_t start, size_t count) {
    if (start + count >= npages) return -1;     /* out of page frame range */

    for (unsigned int i=0; i<count; ++i) {
        if (get_frame_map(start + i))
            return -1;
    }
    return 0;
}

struct page_alloc alloc_pages(size_t count) {
    struct page_alloc pa = {0, 0};
    if (count >= npages || count == 0)
        return pa;
    for (unsigned int i=0; i <= npages - count; ++i) {
        if (check_continuous_free_page(i, count) == 0) {
            for (unsigned int j=0; j<count; ++j)
                set_frame_map(i + j, 1);
            pa.page = (char*)startframe + (i * PAGE_SIZE);
            pa.npages = count;
            return pa;
        }
    }
    return pa;
}

void free_pages(struct page_alloc *pa) {
    if (pa->npages != 0) {
        unsigned int start = (pa->page - startframe) / PAGE_SIZE;
        for (unsigned int i=start; i<start + pa->npages; ++i)
            set_frame_map(i, 0);
    }
}

static int do_mmap(uint64_t virtaddr) {
    /* check mappable region */
    if (virtaddr < (uint64_t)&_kernel_end - HIGHER_HALF_OFFSET || virtaddr > HIGHER_HALF_OFFSET) {
        printk("Invalid virtual address to map: %x\n", virtaddr);
        hlt();
    }

    int r = 0;
    struct mm_struct *mm = current_task_TCB->mm;

    /* calculate page indices */
    unsigned int pml4_idx, pdptr_idx, pd_idx, pt_idx;
    virtaddr2page(virtaddr, &pml4_idx, &pdptr_idx, &pd_idx, &pt_idx);
    printk("pml4_idx: %u, pdptr_idx: %u, pd_idx: %u, pt_idx: %u\n", pml4_idx, pdptr_idx, pd_idx, pt_idx);
    
    uint64_t *p_pml4 = (uint64_t*)((uint64_t)(mm->pgd & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET);
    /* check and alloc ptpdr page */
    if (p_pml4[pml4_idx] == 0) {
        struct page_alloc pa = alloc_pages(1);
        if (pa.npages == 0) {
            printk("\nFailed to alloc page of pdptr\n");
            hlt();
        }
        p_pml4[pml4_idx] = get_physaddr(p_pml4, pa.page) + 0x07;          /* writable, user, present */
    }


    uint64_t *p_pdptr = (uint64_t*)((p_pml4[pml4_idx] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET);

    /* check and alloc pdptr page */
    if (p_pdptr[pdptr_idx] == 0) {
        struct page_alloc pa = alloc_pages(1);
        if (pa.npages == 0) {
            printk("\nFailed to alloc page of pd\n");
            hlt();
        }
        p_pdptr[pdptr_idx] = get_physaddr(p_pml4, pa.page) + 0x07;
    }
    uint64_t *p_pd = (uint64_t*)((p_pdptr[pdptr_idx] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET);

    /* check and alloc pd page */
    if (p_pd[pd_idx] == 0) {
        struct page_alloc pa = alloc_pages(1);
        if (pa.npages == 0) {
            printk("\nFailed to alloc page of pt\n");
            hlt();
        }
        p_pd[pd_idx] = get_physaddr(p_pml4, pa.page) + 0x07;
    }
    uint64_t *p_pt =  (uint64_t*)((p_pd[pd_idx] & (~PAGE_ADDR_MASK)) + HIGHER_HALF_OFFSET);

    /* check and alloc pt page */
    if (p_pt[pt_idx] == 0) {
        struct page_alloc pa = alloc_pages(1);
        if (pa.npages == 0) {
            printk("\nFailed to alloc page of pt\n");
            hlt();
        }
        p_pt[pt_idx] = get_physaddr(p_pml4, pa.page) + 0x07;
    }

    /* do load cr3 */
    setcr3(mm->pgd);

    return 0;
}

void page_fault_handler(unsigned long error_code) {
    uint64_t address = getcr2();
    
    /* check error code */
    /* notice bit W indicates r or w op, skip here*/
    printk("error_code: %u\n", error_code);
    if (((error_code & 0b1) == 0)           /* bit P not set */
    && ((error_code & 0b100) != 0)          /* bit U set */ 
    && ((error_code & (~0b111)) == 0)) {    /* other bit not set */
        do_mmap(address);

        // do page fault, remap page
        // panic("Do page fault, error: %u, address: %x\n", error_code, address);
        return;
    } else {
        panic("Unknown page fault error: %u, address: %x\n", error_code, address);
    }
    
    __asm__ volatile ("hlt");
}
