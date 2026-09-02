#include "../sched/task.h"

extern struct thread_control_block *current_task_TCB;
extern void kernel_idle_work(void);
task_id_t sys_fork() {

    /* create new task */

    struct thread_control_block *new_task = create_task(kernel_idle_work);

    return 0;
}