#include "kernel/list.h"
#include "kernel/page.h"
#include "kernel/timer.h"
#include "kernel/mm.h"
#include "kernel/tty.h"
#include "pagemanager.h"
#include "task.h"

#define offset_of(type, member) \
    (uint64_t)&(((type *)0)->member)

#define TIME_SLICE_LENGTH     20000

#define TCB_MEM_SIZE 1024
static char tcb_mem[TCB_MEM_SIZE]; /* memory for tcb */

const uint64_t TCB_rsp_offset = offset_of(struct thread_control_block, rsp);
const uint64_t TCB_rsp0_offset = offset_of(struct thread_control_block, rsp0);
const uint64_t TCB_state_offset = offset_of(struct thread_control_block, state);
const uint64_t TCB_cr3_offset = offset_of(struct thread_control_block, cr3);

struct thread_control_block *current_task_TCB = NULL;
struct list_head *ready_tcb_list = NULL;
struct list_head *paused_task_list = NULL;
struct list_head *sleeping_task_list = NULL;
struct list_head *terminated_task_list = NULL;
unsigned long last_count = 0;

/* for task postpone */
int postpone_task_switches_counter = 0;
int task_switches_postponed_flag = 0;

/* for time accounting for task switching */
unsigned long time_slice_remaining = 0;

void kernel_idle_work(void) {
    for(;;) {
        size_t when = 0;
        if (sleeping_task_list) {
            struct thread_control_block *task = container_of(sleeping_task_list, struct thread_control_block, tcb_list);
            when = task->sleep_expiry;
        }


        printf("idle work {%u %u} ", when, get_timer_count());
        printf("(");
        if (ready_tcb_list) {
            struct list_head *p = ready_tcb_list;
            do {
                printf("%u ", (container_of(p, struct thread_control_block, tcb_list))->task_id);
                p = p->next;
            } while (p != ready_tcb_list);
        }
        printf(") ");

        for (unsigned long i=0; i<300000000; ++i);
        // lock_scheduler();
        // schedule();
        // unlock_scheduler();
    }
}
struct thread_control_block *kernel_idle_task = NULL;

void kernel_clean_work(void) {
    struct thread_control_block *task = NULL;
    lock_stuff();

    while (terminated_task_list != NULL) {
        task = container_of(terminated_task_list, struct thread_control_block, tcb_list);
        if (terminated_task_list == terminated_task_list->next)
            terminated_task_list = NULL;
        else {
            terminated_task_list = terminated_task_list->next;
            list_del(&task->tcb_list);
        }
        printf("task %u terminated\n", task->task_id);
        kfree_frame(task->rsp0-PAGE_SIZE);
        kfree(task);
    }
    block_task(PAUSED);
    unlock_stuff();
}
struct thread_control_block *kernel_clean_task = NULL;

static void task_start_up() {
    unlock_scheduler();
}

void init_scheduler(void) {

    /* init task */
    kmemory_init(tcb_mem,TCB_MEM_SIZE);
    kernel_idle_task = (struct thread_control_block*)kmalloc(sizeof(struct thread_control_block));
    kernel_idle_task->task_id = 0;
    kernel_idle_task->rsp0 = 0; // kalloc_frame() + PAGE_SIZE;
    kernel_idle_task->rsp =  current_task_TCB->rsp0;
    kernel_idle_task->cr3 = getcr3();
    kernel_idle_task->state = RUNNING;
    kernel_idle_task->time_used = 0;
    current_task_TCB = kernel_idle_task;
    last_count = get_timer_count();
    time_slice_remaining = TIME_SLICE_LENGTH;

    kernel_clean_task = create_task(kernel_clean_work);
    ready_tcb_list = NULL;
    paused_task_list = &kernel_clean_task->tcb_list;
    INIT_LIST_HEAD(paused_task_list);
}

#define PUSH_STACK(s, v) \
    s-=sizeof(uint64_t);*(uint64_t*)(s)=v
struct thread_control_block *create_task(void (*ent)) {
        static unsigned long task_id_counter = 0;

        /* alloc new tcb mem */
        struct thread_control_block *new_task = (struct thread_control_block *)kmalloc(sizeof(struct thread_control_block));
        if (!new_task)
            return 0;

        /* init new task */
        new_task->task_id = ++task_id_counter;
        new_task->rsp0 = kalloc_frame() + PAGE_SIZE;
        new_task->rsp =  new_task->rsp0;
        new_task->cr3 = getcr3();
        new_task->state = READY;
        new_task->time_used = 0;


        /* init stack */
        PUSH_STACK(new_task->rsp, ent); /* ret function */
        PUSH_STACK(new_task->rsp, task_start_up);
        PUSH_STACK(new_task->rsp, 0);   /* rax */
        PUSH_STACK(new_task->rsp, 0);   /* rbx */
        PUSH_STACK(new_task->rsp, 0);   /* rcx */
        PUSH_STACK(new_task->rsp, 0);   /* rsi */

        /* add to ready list */
        if (!ready_tcb_list) {
            ready_tcb_list = &(new_task->tcb_list);
            INIT_LIST_HEAD(ready_tcb_list);
        } else {
            list_add_tail(&new_task->tcb_list, ready_tcb_list);
        }
        return new_task;
}

void schedule() {
    if (postpone_task_switches_counter != 0) {
        /* 此处流程通常是因为之前调用了lock_stuff，此处会跳过当前schedule，推迟到unblock_stuff中的schedule */
        /* 此处目的是上下文切换与调度的分离 */
        task_switches_postponed_flag = 1;
        return;
    }

    if (ready_tcb_list) {
        if (current_task_TCB->state == RUNNING && current_task_TCB != kernel_idle_task) {
            current_task_TCB->state = READY;
            list_add_tail(&current_task_TCB->tcb_list, ready_tcb_list);
        }
        struct thread_control_block *next_task = container_of(ready_tcb_list, struct thread_control_block, tcb_list);
        next_task->state = RUNNING;
        if (ready_tcb_list->next == ready_tcb_list)
            ready_tcb_list = NULL;
        else
            ready_tcb_list = ready_tcb_list->next;
        list_del(&next_task->tcb_list);
        time_slice_remaining = TIME_SLICE_LENGTH;
        switch_to_task(next_task);
    } else {
        if (current_task_TCB->state == RUNNING)
            return;
        // terminal_writestring("No tasks to switch!");
        // while (1);
        time_slice_remaining = 0;
        switch_to_task(kernel_idle_task);
    }

}

void update_time_used(void) {
    unsigned long current_count = get_timer_count();
    unsigned long elapsed = current_count - last_count;
    last_count = current_count;
    current_task_TCB->time_used += elapsed;
}

int irq_disable_counter = 0;
void lock_scheduler() {
    cli();
    irq_disable_counter++;
}

void unlock_scheduler() {
    irq_disable_counter--;
    if (irq_disable_counter == 0) {
        sti();
    }
}

void block_task(state_t reason) {
    lock_scheduler();
    if (current_task_TCB == kernel_idle_work) {
        unlock_scheduler();
        return;
    }
    
    current_task_TCB->state = reason;
    struct list_head **current_task_list = NULL;
    switch (reason)
    {
    case PAUSED:             /* 被阻塞的任务 */
        current_task_list = &paused_task_list;
        break;
    case SLEEPING:           /* 休眠的任务 */
        current_task_list = &sleeping_task_list;
        break;
    case TERMINATED:         /* 被终止的任务 */
        current_task_list = &terminated_task_list;
        break;
    case WAITING_FOR_LOCK:
        break;
    default:
        unlock_scheduler();
        return;
    }

    if (current_task_list) {
        if (!(*current_task_list)) {
            *current_task_list = &current_task_TCB->tcb_list;
            INIT_LIST_HEAD(*current_task_list);
        } else
            list_add_tail(&current_task_TCB->tcb_list, *current_task_list);
    }

    schedule();
    unlock_scheduler();
}

void unblock_task(struct thread_control_block *task) {
    lock_scheduler();

    /* traverse these blocked list */
    struct list_head **blocked_lists[] = {&sleeping_task_list, &paused_task_list};
    for (unsigned int i = 0; i < sizeof(blocked_lists)/sizeof(blocked_lists[0]); ++i) {
        if (&task->tcb_list == (*blocked_lists[i])) {  /* if the task stands first at blocked_list */
            if ((*blocked_lists[i])->next == *blocked_lists[i])
                *blocked_lists[i] = NULL;
            else
                *blocked_lists[i] = (*blocked_lists[i])->next;
        }
    }

    list_del(&task->tcb_list);
    task->state = READY;
    if (ready_tcb_list) {
        list_add_tail(&task->tcb_list, ready_tcb_list);
    } else {
        ready_tcb_list = &task->tcb_list;
        INIT_LIST_HEAD(ready_tcb_list);
    }
    unlock_scheduler();
}

void lock_stuff(void) {
#ifndef SMP
    cli();
    irq_disable_counter++;
    postpone_task_switches_counter++;
#endif
}

void unlock_stuff(void) {
#ifndef SMP
    postpone_task_switches_counter--;
    if (postpone_task_switches_counter == 0) {
        if (task_switches_postponed_flag != 0) {
            task_switches_postponed_flag = 0;
            schedule();
        }
    }

    irq_disable_counter--;
    if (irq_disable_counter == 0) {
        sti();
    }
#endif
}

void nano_sleep_until(uint64_t when) {

    lock_stuff();

    if (when < get_timer_count()) {
        unlock_stuff();
        return;
    }
    current_task_TCB->sleep_expiry = when;
    block_task(SLEEPING);
    unlock_stuff();
}

void terminate_task(void) {
    lock_stuff();
    unblock_task(kernel_clean_task);
    block_task(TERMINATED);
    unlock_stuff();
}

// 定义一个函数，用于定时器处理程序中的任务钩子
void task_hook_in_timer_handler(void) {
    lock_scheduler();

    if (sleeping_task_list) {
        struct list_head *anchor_task_list = sleeping_task_list;
        do {
            struct thread_control_block *task = container_of(sleeping_task_list, struct thread_control_block, tcb_list);
            while (task->sleep_expiry <= get_timer_count()) {
                unblock_task(task);
                task = container_of(sleeping_task_list, struct thread_control_block, tcb_list);
                anchor_task_list = sleeping_task_list;
            }
            /* 到此处可以判断当前的sleeping_task_list时间未到 */

            if (!sleeping_task_list) break;
            sleeping_task_list = sleeping_task_list->next;

        } while(anchor_task_list != sleeping_task_list); 

    }

    if (ready_tcb_list) {
        if (time_slice_remaining <=1)
            /* 首次schedule，对应下一个任务的start_up会执行unlock_scheduler */
            /* 之后的其他次切换，执行的都是下文中的unlock_scheduler */
            schedule();
        else
            time_slice_remaining--;
    }

    unlock_scheduler();
}