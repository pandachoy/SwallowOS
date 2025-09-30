#ifndef _PAGEMANAGER_H
#define _PAGEMANAGER_H

#include <stdint.h>

/* 页帧格式 */
typedef uint64_t *pageframe_t;

struct page_alloc {
    pageframe_t page;
    uint64_t npages;
};

pageframe_t kalloc_frame_init();
pageframe_t kalloc_frame();
void kfree_frame(pageframe_t a);

struct page_alloc alloc_pages(size_t size);

#endif