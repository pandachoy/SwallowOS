#ifndef _PAGE_H
#define _PAGE_H

#include <stdint.h>

/* 页帧格式 */
typedef uint32_t *pageframe_t;

pageframe_t kalloc_frame_init();
pageframe_t kalloc_frame();
void kfree_frame(pageframe_t a);

#endif