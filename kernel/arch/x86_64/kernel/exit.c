#include "../mm/pagemanager.h"
#include <kernel/malloc.h>
#include "../sched/task.h"
#include <kernel/list.h>

void do_exit(int code) {
    struct mm_struct *mm;


    mm = current_task_TCB->mm;
    if (!mm) return;

    /* ummap */
    setcr3(kernel_pgd);
    do_ummap_user(mm->pgd);

    /* clean mm */
    mm_clean(mm);
    kfree(mm);
    current_task_TCB->mm = NULL;

    /* terminate current task */
    terminate_task();
}

int sys_exit(int code) {

    do_exit(code);

    return 0;
}

