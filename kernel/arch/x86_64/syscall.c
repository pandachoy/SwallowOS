#include <stddef.h>
#include <stdio.h>
#include "task.h"
#include "syscall.h"

extern struct thread_control_block *current_task_TCB;

int sys_read(int fd, size_t size, char *buffer) {
    printf("read called %u ", ((uint64_t)current_task_TCB->rsp0 | 0xFFF) - (uint64_t)current_task_TCB->rsp0);
    return 1;
}

int sys_write(int fd, size_t size, char *buffer) {
    printf("write called %d ", (int)buffer);
    return 2;
}

uint64_t sys_get_task_id() {
    return current_task_TCB->task_id;
}

uint64_t sys_get_rsp0() {
    return current_task_TCB->rsp0;
}

void * syscalls[] = {
    sys_read,
    sys_write,
    sys_get_task_id,
    sys_get_rsp0
};

