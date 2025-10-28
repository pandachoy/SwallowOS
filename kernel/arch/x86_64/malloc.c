#include <stdint.h>
#include <kernel/malloc.h>
#include <kernel/tty.h>
#include <kernel/page.h>
#include <kernel/printk.h>
#include "pagemanager.h"

#define HEADER_SIZE       ((size_t)&((chunk*)0)->data)
#define MM_PAGE_NUM                               1024

#define is_last_entry(h) ((h)->next==(h))

typedef struct {
    pageframe_t page;
    size_t mem_free;
    size_t mem_used;
    size_t mem_meta;
    chunk *first, *last;
    chunk *free_chunk[NUM_SIZES];
    struct list_head *prev, *next;
} page_tag;

page_tag mm_pages[MM_PAGE_NUM] = { NULL };
int cur_page_tag_index = -1;
static size_t max_page_free_size = PAGE_SIZE;    /* 存储一个page中的最大可能free size */

/* 初始化一个chunk */
static void memory_chunk_init(chunk *ck) {
    INIT_LIST_HEAD(&ck->all);
    ck->used = 0;
    ck->page_tag_index = cur_page_tag_index;
    INIT_LIST_HEAD(&ck->free);
}

/* 获取chunk大小，实际空间的大小 */
static size_t memory_chunk_size(const chunk *ck) {
    char *end = (char*)(ck->all.next);
    char *start = (char*)(&ck->all);
    return (end - start);
}

/* 定位size对应的slot */
static int memory_chunk_slot(size_t size) {
    int n = -1;
    while (size > 0) {
        ++n;
        size /= 2;
    }
    return n;
}

void kmemory_init() {
    for (unsigned int i = 0; i < MM_PAGE_NUM * sizeof(page_tag); ++i) {
        *((char*)mm_pages + i) = 0;
    }
}

static void kmemory_page_init(unsigned int page_tag_index) {

    if (page_tag_index >= MM_PAGE_NUM) return;
    page_tag *pt = &mm_pages[page_tag_index];

    void *mem = pt->page;
    char *mem_start = (char*)(((intptr_t)mem + ALIGN -1) & (~(ALIGN -1)));        /* 向后对齐 */
    char *mem_end = (char*)(((intptr_t)mem + PAGE_SIZE) & (~(ALIGN - 1)));             /* 向前对齐 */

    pt->first = (chunk*)mem_start;                                                    /* 第一个块 */
    chunk *second = pt->first + 1;
    pt->last = ((chunk*)mem_end) - 1;                                                 /* 最后一个块 */
    memory_chunk_init(pt->first);
    memory_chunk_init(second);
    memory_chunk_init(pt->last);
    list_add_tail(&second->all, &pt->first->all);
    list_add_tail(&pt->last->all, &pt->first->all);
    /* make first/last as used so they never get merged */
    pt->first->used = 1;
    pt->last->used = 1;
    pt->first->page_tag_index = page_tag_index;
    pt->last->page_tag_index = page_tag_index;
    second->page_tag_index = page_tag_index;


    size_t len = memory_chunk_size(second);
    int n = memory_chunk_slot(len);
    // printk("%s(%d, %d): adding chunk %d %d\n", __FUNCTION__, mem, size, len, n);
    pt->free_chunk[n] = second;
    pt->mem_free = len - HEADER_SIZE;                                                /* 为什么这里还要减去一个HEADER_SIZE?在memory_chunk_size已经减去一个了 */
    max_page_free_size = max_page_free_size < pt->mem_free ? max_page_free_size : pt->mem_free;
    pt->mem_meta = sizeof(chunk) * 2 + HEADER_SIZE;
}

static void *kmalloc_on_cur_page(size_t size) {
    if (cur_page_tag_index == -1) return NULL;
    page_tag *cur_page_tag = &(mm_pages[cur_page_tag_index]);

    size = (size + ALIGN - 1) & (~(ALIGN - 1));

    if (size < MIN_SIZE) size = MIN_SIZE;

    if (size > cur_page_tag->mem_free) return NULL;

    int n = memory_chunk_slot(size + HEADER_SIZE);
    if (n >= NUM_SIZES) return NULL;                                               /* 无法分配超出范围的内存 */
    while (!cur_page_tag->free_chunk[n]) {                                                       /* 都是从free_chunk中查找的，类似glibc中的各种bin？ */
        ++n;
        if (n >= NUM_SIZES) return NULL;
    }

    chunk *fetch_ck = cur_page_tag->free_chunk[n];
    if (is_last_entry(&fetch_ck->free))
        cur_page_tag->free_chunk[n] = NULL;
    else {
        cur_page_tag->free_chunk[n] = container_of(fetch_ck->free.next, chunk, free);
        list_del(&fetch_ck->free);                                                  /* 从list中取出*/
    }
    size_t size2 = memory_chunk_size(fetch_ck);
    size_t len = 0;

    if (size + 2 * sizeof(chunk) <= size2) {      /* 注意，这里比较时要用size+2*sizeof(chunk)，因为后续的chunk包括fetch_ck和ck2的chunk */
        chunk *ck2 = (chunk*)(((char*)fetch_ck) + HEADER_SIZE + size);             /* 此处把ck2作为切分之后余下的chunk添加到all list和free list */
        memory_chunk_init(ck2);
        list_add(&ck2->all, &fetch_ck->all);    /* ck2插入到fetch_ck后面 */
        len = memory_chunk_size(ck2);

        int n = memory_chunk_slot(len);
        if (!cur_page_tag->free_chunk[n]) {
            INIT_LIST_HEAD(&ck2->free);
            cur_page_tag->free_chunk[n] = ck2;
        } else
            list_add_tail(&ck2->free, &cur_page_tag->free_chunk[n]->free);
        cur_page_tag->mem_meta += HEADER_SIZE;                                                   /* 只有all list变化时mem_meta才会改变 */
        cur_page_tag->mem_free += len - HEADER_SIZE;                                             /* 只有free_list变化时mem_free会发生改变 */
    }

    fetch_ck->used = 1;
    cur_page_tag->mem_free -= (size2 - HEADER_SIZE);
    cur_page_tag->mem_used += size2 - len - HEADER_SIZE;                                          /* 当新增used chunk时，mem_used会改变 */
    
    if (cur_page_tag->mem_used + cur_page_tag->mem_free + cur_page_tag->mem_meta != max_page_free_size + sizeof(chunk) * 2 + HEADER_SIZE) {
        printk("panic");
    }

    return fetch_ck->data;
}

void *kmalloc(size_t size) {
    if (size == 0 || size > max_page_free_size) return NULL;


    /* 首次分配需要设置当前的page_tag */
    if (cur_page_tag_index == -1) {
        pageframe_t pf = kalloc_frame();
        if (!pf) return NULL;
        cur_page_tag_index = 0;
        mm_pages[cur_page_tag_index].page = pf;
        kmemory_page_init(0);
    }

    /* 在当前页分配内存 */
    void *p = kmalloc_on_cur_page(size);

    /* 当前页无法满足，则在列表里搜索新页 */
    if (!p) {
        int first_free_index = -1;  /* 记录首个空page位置，以便后续发生的page分配 */
        for (unsigned int i = 0; i < MM_PAGE_NUM; ++i) {
            page_tag *tmp_pt = mm_pages + i;
            if (i == cur_page_tag_index)
                continue;
            if (first_free_index == -1 && tmp_pt->page == 0) {
                first_free_index = i;
                continue;
            }
            if (tmp_pt->mem_free < size)
                continue;
            cur_page_tag_index = i;
            p = kmalloc_on_cur_page(size);
            if (p)
                break;
        }
        /* 如果在所有已申请页还未分配内存，则申请新页 */
        if (!p && (first_free_index != -1)) {
            pageframe_t pf = kalloc_frame();
            if (!pf) return NULL;
            mm_pages[first_free_index].page = pf;
            cur_page_tag_index = first_free_index;
            kmemory_page_init(cur_page_tag_index);
            p = kmalloc_on_cur_page(size);
        }
    }

    int check = kmcheck();
    if (check != 0) {
        printk("check ");
    }

    return p;
}

static void remove_free(chunk *ck) {
    page_tag *pt = &(mm_pages[ck->page_tag_index]);
    size_t len = memory_chunk_size(ck);
    int n = memory_chunk_slot(len);
    if (is_last_entry(&(pt->free_chunk[n]->free))) {
        if (ck == pt->free_chunk[n])
            pt->free_chunk[n] = NULL;
    } else {
        pt->free_chunk[n] = container_of(ck->free.next, chunk, free);
        list_del(&ck->free);
    }

    pt->mem_free -= len - HEADER_SIZE;
}

static void push_free(chunk *ck) {
    page_tag *pt = &(mm_pages[ck->page_tag_index]);
    size_t len = memory_chunk_size(ck);
    int n = memory_chunk_slot(len);
    if (!pt->free_chunk[n]) {
        INIT_LIST_HEAD(&ck->free);
        pt->free_chunk[n] = ck;
    } else {
        list_add_tail(&ck->free, &pt->free_chunk[n]->free);
    }
}

void kfree(void *mem) {
    chunk *ck = (chunk*)((char*)mem - HEADER_SIZE);
    chunk *next = container_of(ck->all.next, chunk, all);
    chunk *prev = container_of(ck->all.prev, chunk, all);


    unsigned int page_tag_index = ck->page_tag_index;
    page_tag *pt = &mm_pages[page_tag_index];
    pt->mem_used -= memory_chunk_size(ck);

    /* try to merge */
    if (next->used == 0) {
        remove_free(next);                                        /* 从free list移出next*/
        list_del(&next->all);                                     /* 删除next */
        pt->mem_meta -= HEADER_SIZE;                                  /* mem_meta少了next的header大小 */
        pt->mem_free += HEADER_SIZE;                                  /* mem_free多了next的header大小 */
    }

    if (prev->used == 0) {
        remove_free(prev);                                        /* 从free list移出prev */
        list_del(&ck->all);                                       /* 删除当前ck，因为要和prev合并 */
        push_free(prev);                                          /* 将新的prev放入free list */
        pt->mem_meta -= HEADER_SIZE;
        pt->mem_free -= HEADER_SIZE;
    } else {
        ck->used = 0;
        INIT_LIST_HEAD(&ck->free);
        push_free(ck);                                             /* 将free后的ck重新放入free list */
    }

    /* 如果page中无其他释放page */
    struct list_head *p = pt->first->all.next;
    int need_free_page = 1;
    while (p != &pt->last->all) {
        chunk *tmp = container_of(p, chunk, all);
        if (tmp->used) {
            need_free_page = 0;
            break;
        }
        p = p->next;
    }
    if (need_free_page) {
        kfree_frame(pt->page);
        pt->page = 0;
        // for (unsigned int i = 0; i < MM_PAGE_NUM * sizeof(page_tag); ++i) {
        //     *((char*)mm_pages + i) = 0;
        // }
    }
}

int kmcheck(void) {
    for (unsigned int i=0; i<MM_PAGE_NUM; ++i) {
        page_tag *pt = &(mm_pages[i]);
        if (pt->page == 0) continue;

        chunk *t = pt->last;

        struct list_head *it = &pt->first->all;
        do {
            chunk *tmp = container_of(it->prev, chunk, all);
            chunk *tmp1 = container_of(it, chunk, all);
            if (tmp != t) {
                return -1;
            }
            t = tmp1;
            it = it->next;
        } while (it != &pt->first->all);

        for (unsigned int i = 0; i < NUM_SIZES; ++i) {
            if (pt->free_chunk[i]) {
                t = container_of(pt->free_chunk[i]->free.prev, chunk, free);

                struct list_head *it = &pt->free_chunk[i]->free;
                do {
                    chunk *tmp = container_of(it->prev, chunk, free);
                    chunk *tmp1 = container_of(it->prev, chunk, free);
                    if (tmp != t) {
                        return -1;
                    }
                    t = tmp1;
                    it = it->next;
                } while (it != &pt->free_chunk[i]->free);
            }
        }
    }

    return 0;
}

int km_freecheck(void) {
    for (unsigned int i=0; i<MM_PAGE_NUM; ++i) {
        page_tag *pt = &(mm_pages[i]);
        if (pt->page != 0) {
            printk("non freed page found! ");
        }
    }
} 



