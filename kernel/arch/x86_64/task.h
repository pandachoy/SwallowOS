#ifndef _THREAD_H
#define _THREAD_H

#include <stdint.h>

struct thread_control_block {
    void* rsp;                              /* the task's kernel stack */
    void* rsp0;                             /* top of kernel stack */ 
    uint32_t cr3;                              /* the task's virtual address space*/
    struct thread_control_block *next;      /* next task field */
    uint8_t state;                          /* state field */
    unsigned long used;
};

extern void switch_to_task(struct thread_control_block *next_thread);
extern unsigned int getcr3();
void init_scheduler(void);
struct thread_control_block *create_task(void (*ent));
void schedule();

extern struct thread_control_block *current_task_TCB;


extern const uint64_t TCB_rsp_offset;
extern const uint64_t TCB_rsp0_offset;
extern const uint64_t TCB_cr3_offset;
extern const uint64_t TCB_next_offset;
extern const uint64_t TCB_state_offset;

#endif