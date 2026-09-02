#include "../mm/pagemanager.h"
#include <kernel/malloc.h>
#include "../sched/task.h"
#include <kernel/list.h>

void do_exit(int code) {
    (void)code;
    if (!current_task_TCB->mm) return;
    terminate_task();
}

int sys_exit(int code) {

    do_exit(code);

    return 0;
}
