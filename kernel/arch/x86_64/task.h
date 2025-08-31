#ifndef _THREAD_H
#define _THREAD_H

#include <stdint.h>
#include <kernel/list.h>

typedef enum {
    RUNNING,
    READY,
    PAUSED,
    SLEEPING,
    TERMINATED,
    WAITING_FOR_LOCK
} state_t;

struct thread_control_block {
    unsigned long task_id;
    void* rsp;                              /* the task's kernel stack */
    void* rsp0;                             /* top of kernel stack */ 
    uint32_t cr3;                              /* the task's virtual address space*/
    // struct thread_control_block *next;      /* next task field */
    state_t state;                          /* state field */
    unsigned long time_used;
    unsigned long sleep_expiry;

    struct list_head tcb_list;
};

extern void switch_to_task(struct thread_control_block *next_thread);
extern unsigned int getcr3();
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


extern const uint64_t TCB_rsp_offset;
extern const uint64_t TCB_rsp0_offset;
extern const uint64_t TCB_cr3_offset;
extern const uint64_t TCB_state_offset;

#endif