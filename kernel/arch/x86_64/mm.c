#include <stdint.h>
#include <kernel/mm.h>
#include <kernel/tty.h>


#define HEADER_SIZE       ((size_t)&((chunk*)0)->data)

#define is_last_entry(h) ((h)->next==(h))

chunk *free_chunk[NUM_SIZES] = { NULL };
size_t mem_free = 0;
size_t mem_used = 0;
size_t mem_meta = 0;
chunk *first = NULL;
chunk *last = NULL;

/* 初始化一个chunk */
static void memory_chunk_init(chunk *ck) {
    INIT_LIST_HEAD(&ck->all);
    ck->used = 0;
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

void kmemory_init(void *mem, size_t size) {

    char *mem_start = (char*)(((intptr_t)mem + ALIGN -1) & (~(ALIGN -1)));        /* 向后对齐 */
    char *mem_end = (char*)(((intptr_t)mem + size) & (~(ALIGN - 1)));             /* 向前对齐 */

    first = (chunk*)mem_start;                                                    /* 第一个块 */
    chunk *second = first + 1;
    last = ((chunk*)mem_end) - 1;                                                 /* 最后一个块 */
    memory_chunk_init(first);
    memory_chunk_init(second);
    memory_chunk_init(last);
    list_add_tail(&second->all, &first->all);
    list_add_tail(&last->all, &first->all);
    /* make first/last as used so they never get merged */
    first->used = 1;
    last->used = 1;


    size_t len = memory_chunk_size(second);
    int n = memory_chunk_slot(len);
    // printf("%s(%d, %d): adding chunk %d %d\n", __FUNCTION__, mem, size, len, n);
    free_chunk[n] = second;
    mem_free = len - HEADER_SIZE;                                                  /* 为什么这里还要减去一个HEADER_SIZE?在memory_chunk_size已经减去一个了 */
    mem_meta = sizeof(chunk) * 2 + HEADER_SIZE;
}

void *kmalloc(size_t size) {
    size = (size + ALIGN - 1) & (~(ALIGN - 1));

    if (size < MIN_SIZE) size = MIN_SIZE;

    int n = memory_chunk_slot(size - 1) + 1;
    if (n >= NUM_SIZES) return NULL;                                               /* 无法分配超出范围的内存 */
    while (!free_chunk[n]) {                                                       /* 都是从free_chunk中查找的，类似glibc中的各种bin？ */
        ++n;
        if (n >= NUM_SIZES) return NULL;
    }

    chunk *fetch_ck = free_chunk[n];
    if (is_last_entry(&fetch_ck->free))
        free_chunk[n] = NULL;
    else {
        free_chunk[n] = container_of(fetch_ck->free.next, chunk, free);
        list_del(&fetch_ck->free);                                                  /* 从list中取出*/
    }
    size_t size2 = memory_chunk_size(fetch_ck);
    size_t len = 0;

    if (size + sizeof(chunk) <= size2) {
        chunk *ck2 = (chunk*)(((char*)fetch_ck) + HEADER_SIZE + size);             /* 此处把ck2作为切分之后余下的chunk添加到all list和free list */
        memory_chunk_init(ck2);
        list_add(&ck2->all, &fetch_ck->all);
        len = memory_chunk_size(ck2);
        int n = memory_chunk_slot(len);
        if (!free_chunk[n]) {
            INIT_LIST_HEAD(&ck2->free);
            free_chunk[n] = ck2;
        } else
            list_add_tail(&ck2->free, &free_chunk[n]->free);
        mem_meta += HEADER_SIZE;                                                   /* 只有all list变化时mem_meta才会改变 */
        mem_free += len - HEADER_SIZE;                                             /* 只有free_list变化时mem_free会发生改变 */
    }

    fetch_ck->used = 1;
    mem_free -= size2;
    mem_used += size2 - len - HEADER_SIZE;                                          /* 当新增used chunk时，mem_used会改变 */
    return fetch_ck->data;
}

static void remove_free(chunk *ck) {
    size_t len = memory_chunk_size(ck);
    int n = memory_chunk_slot(len);
    if (is_last_entry(&free_chunk[n]->free)) {
        if (ck == free_chunk[n])
            free_chunk[n] = NULL;
    } else {
        free_chunk[n] = container_of(ck->free.next, chunk, free);
        list_del(&ck->free);
    }

    mem_free -= len - HEADER_SIZE;
}

static void push_free(chunk *ck) {
    size_t len = memory_chunk_size(ck);
    int n = memory_chunk_slot(len);
    if (!free_chunk[n]) {
        INIT_LIST_HEAD(&ck->free);
        free_chunk[n] = ck;
    } else {
        list_add_tail(&ck->free, &free_chunk[n]->free);
    }
}

void kfree(void *mem) {
    chunk *ck = (chunk*)((char*)mem - HEADER_SIZE);
    chunk *next = container_of(ck->all.next, chunk, all);
    chunk *prev = container_of(ck->all.prev, chunk, all);

    mem_used -= memory_chunk_size(ck);

    /* try to merge */
    if (next->used == 0) {
        remove_free(next);                                        /* 从free list移出next*/
        list_del(&next->all);                                     /* 删除next */
        mem_meta -= HEADER_SIZE;                                  /* mem_meta少了next的header大小 */
        mem_free += HEADER_SIZE;                                  /* mem_free多了next的header大小 */
    }

    if (prev->used == 0) {
        remove_free(prev);                                        /* 从free list移出prev */
        list_del(&ck->all);                                       /* 删除当前ck，因为要和prev合并 */
        push_free(prev);                                          /* 将新的prev放入free list */
        mem_meta -= HEADER_SIZE;
        mem_free -= HEADER_SIZE;
    } else {
        ck->used = 0;
        INIT_LIST_HEAD(&ck->free);
        push_free(ck);                                             /* 将free后的ck重新放入free list */
    }
}

int kmcheck(void) {
    chunk *t = last;

    struct list_head *it = &first->all;
    do {
        chunk *tmp = container_of(it->prev, chunk, all);
        chunk *tmp1 = container_of(it, chunk, all);
        if (tmp != t) {
            return -1;
        }
        t = tmp1;
        it = it->next;
    } while (it != &first->all);

    for (unsigned int i = 0; i < NUM_SIZES; ++i) {
        if (free_chunk[i]) {
            t = container_of(free_chunk[i]->free.prev, chunk, free);

            struct list_head *it = &free_chunk[i]->free;
            do {
                chunk *tmp = container_of(it->prev, chunk, free);
                chunk *tmp1 = container_of(it->prev, chunk, free);
                if (tmp != t) {
                    return -1;
                }
                t = tmp1;
                it = it->next;
            } while (it != &free_chunk[i]->free);
        }
    }
    return 0;
}



