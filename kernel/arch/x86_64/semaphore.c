#include "kernel/semaphore.h"
#include "kernel/malloc.h"
#include "pagemanager.h"
#include "task.h"

struct semaphore* create_semaphore(unsigned int max_count) {
    struct semaphore *smph;

    smph = (struct semaphore*)kmalloc(sizeof(struct semaphore));
    if (smph != NULL) {
        smph->max_count = max_count;
        smph->current_count = 0;
        smph->waiting_task_list = NULL;
    }
    return smph;
}

struct semaphore* create_mutex(void) {
    return create_semaphore(1);
}

void acquire_semaphore(struct semaphore *smph) {
    lock_stuff();
    if (smph->current_count < smph->max_count) {      /* 不阻塞 */
        smph->current_count++;
    } else {
        if (smph->waiting_task_list == NULL) {
            smph->waiting_task_list = &current_task_TCB->tcb_list;
            INIT_LIST_HEAD(smph->waiting_task_list);
        } else {
            list_add_tail(&current_task_TCB->tcb_list, smph->waiting_task_list);
        }
        block_task(WAITING_FOR_LOCK);
    }
    unlock_stuff();
}

void acquire_mutex(struct semaphore *smph) {
    acquire_semaphore(smph);
}


void release_semaphore(struct semaphore *smph) {
    lock_stuff();
    if (smph->waiting_task_list != NULL) {
        struct thread_control_block *task = container_of(smph->waiting_task_list, struct thread_control_block, tcb_list);
        if (is_last_entry(smph->waiting_task_list)) {
            smph->waiting_task_list = NULL;
        } else {
            smph->waiting_task_list = smph->waiting_task_list->next;
        }
        unblock_task(task);
    } else {
        if (smph->current_count > 0)
            smph->current_count--;
    }
    unlock_stuff();
}

void release_mutex(struct semaphore *smph) {
    release_semaphore(smph);
}