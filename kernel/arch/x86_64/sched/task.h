#ifndef _TASK_H
#define _TASK_H

#include <stdint.h>
#include <kernel/list.h>
#include "../mm/mm.h"

typedef enum {
    RUNNING,
    READY,
    PAUSED,
    SLEEPING,
    TERMINATED,
    WAITING_FOR_LOCK
} state_t;

extern void *page_map_level4;
extern uint64_t kernel_pgd;

struct thread_control_block {
    unsigned long task_id;

    struct page_alloc stack0;
    void* tss_rsp0;                         /* top of kernel stack to set on tss->rsp0*/ 
    void* rsp0;                             /* kernel stack */
    uint64_t pgd0;                           /* the task's virtual address space*/

    struct mm_struct *mm;
    state_t state;                          /* state field */
    unsigned long time_used;
    unsigned long sleep_expiry;

    struct list_head tcb_list;
};

extern void switch_to_task(struct thread_control_block *next_thread);
void init_scheduler(void);
struct thread_control_block *create_task(void (*ent));
void schedule();
void lock_scheduler();
void unlock_scheduler();
void block_task(state_t reason);
void unblock_task(struct thread_control_block *task);
void lock_stuff(void);
void unlock_stuff(void);
void nano_sleep_until(uint64_t when);
void terminate_task(void);
void task_hook_in_timer_handler(void);
void kernel_idle_work(void);

extern struct thread_control_block *current_task_TCB;

#endif