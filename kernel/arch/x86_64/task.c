#include "kernel/list.h"
#include "kernel/page.h"
#include "pagemanager.h"
#include "task.h"

#define MAX_TASK_COUNT 16


#define offset_of(type, member) \
    (uint64_t)&(((type *)0)->member)

struct thread_control_block tcb_array[MAX_TASK_COUNT];

const uint64_t TCB_rsp_offset = offset_of(struct thread_control_block, rsp);
const uint64_t TCB_rsp0_offset = offset_of(struct thread_control_block, rsp0);
const uint64_t TCB_next_offset = offset_of(struct thread_control_block, next);
const uint64_t TCB_state_offset = offset_of(struct thread_control_block, state);
const uint64_t TCB_cr3_offset = offset_of(struct thread_control_block, cr3);

struct thread_control_block *current_task_TCB;

void init_scheduler(void) {
    for (unsigned int i = 0; i < MAX_TASK_COUNT; ++i)
        tcb_array[i].used = 0;

    /* init task */
    tcb_array[0].used = 1;
    tcb_array[0].rsp0 = 0; // kalloc_frame() + PAGE_SIZE;
    tcb_array[0].rsp =  tcb_array[0].rsp0;
    tcb_array[0].cr3 = getcr3();
    tcb_array[0].state = 0;
    tcb_array[0].next = &tcb_array[0];
    current_task_TCB = &tcb_array[0];
}

#define PUSH_STACK(s, v) \
    s-=sizeof(uint64_t);*(uint64_t*)(s)=v
struct thread_control_block *create_task(void (*ent)) {
        for (unsigned int i = 0; i < MAX_TASK_COUNT; ++i) {
            /* search a free tcb */
            if (tcb_array[i].used == 0) {
                /* init task */
                tcb_array[i].used = 1;
                tcb_array[i].rsp0 = kalloc_frame() + PAGE_SIZE;
                tcb_array[i].rsp =  tcb_array[i].rsp0;
                tcb_array[i].cr3 = getcr3();
                tcb_array[i].state = 0;

                /* add to task list*/
                tcb_array[i].next = tcb_array[0].next;
                tcb_array[0].next = &tcb_array[i];

                /* init stack */
                PUSH_STACK(tcb_array[i].rsp, ent); /* ret function */
                PUSH_STACK(tcb_array[i].rsp, 0);   /* rax */
                PUSH_STACK(tcb_array[i].rsp, 0);   /* rbx */
                PUSH_STACK(tcb_array[i].rsp, 0);   /* rcx */
                PUSH_STACK(tcb_array[i].rsp, 0);   /* rsi */
                return &tcb_array[i];
            }
        }
        return 0;
}

void schedule() {
    switch_to_task(current_task_TCB->next);
}