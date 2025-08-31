#ifndef _KERNEL_SEMAPHORE_H
#define _KERNEL_SEMAPHORE_H

#include <kernel/list.h>

struct semaphore {
    unsigned int max_count;
    unsigned int current_count;
    struct list_head *waiting_task_list;
};

struct semaphore* create_semaphore(unsigned int max_count);
struct semaphore* create_mutex(void);
void acquire_semaphore(struct semaphore *smph);
void acquire_mutex(struct semaphore *smph);
void release_semaphore(struct semaphore *smph);
void release_mutex(struct semaphore *smph);

#endif