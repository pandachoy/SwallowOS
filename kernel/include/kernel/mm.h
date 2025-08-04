#ifndef _MM_H
#define _MM_H

#include <stddef.h>
#include <kernel/list.h>

#define NUM_SIZES                                   32
#define ALIGN                                        4
#define MIN_SIZE              sizeof(struct list_head)

typedef struct {
    struct list_head all;
    int used;
    union {
        char data[0];
        struct list_head free;
    };
} chunk;

void kmemory_init(void *mem, size_t size);
void *kmalloc(size_t size);
void kfree(void *mem);
int kmcheck(void);

#endif