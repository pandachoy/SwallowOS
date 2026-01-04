#include <stddef.h>
#include <kernel/printk.h>
#include <kernel/tty.h>
#include "../sched/task.h"

extern struct thread_control_block *current_task_TCB;

int sys_read(int fd, size_t size, char *buffer) {
    struct mm_struct *mm = current_task_TCB->mm;
    if (!mm) return -1;
    printk("read called %u ", ((uint64_t)current_task_TCB->rsp0 | 0xFFF) - (uint64_t)current_task_TCB->rsp0);
    return 1;
}

int sys_write(int fd, size_t size, char *buffer) {
    printk("write called %d ", (int)buffer);
    return 2;
}

uint64_t sys_get_task_id() {
    // printk("get_task_id called %u ", current_task_TCB->task_id);
    return current_task_TCB->task_id;
}

uint64_t sys_get_rsp0() {
    // printk("get_rsp0 called %u ", current_task_TCB->rsp0);
    return current_task_TCB->mm ? current_task_TCB->rsp0 : 0;
}


int sys_putchar(int ic) {
    terminal_putchar(ic);
    return 0;
}

extern int sys_exit(int code);

void * syscalls[] = {
    sys_read,
    sys_write,
    sys_get_task_id,
    sys_get_rsp0,
    sys_putchar,
    sys_exit
};

