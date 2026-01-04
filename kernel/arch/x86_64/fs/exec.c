#include <os/binfmt.h>
#include <kernel/printk.h>
#include "../sched/task.h"
#include "../mm/mm.h"
#include <kernel/malloc.h>
#include "elfloader.h"

int kernel_exec(fat12_t *fs, const char *filename, const char *const *argv, const char *const *envp) {
    /* set up mm */
    int r;
    struct mm_struct *mm = NULL;
    uint64_t new_pgd = 0;

    lock_scheduler();

    /* check mm */
    if (current_task_TCB->mm) {
        printk("Failed to exec user elf because mm existed\n");
        r = -1; goto out;
    }

    /* create mm */
    mm = kmalloc(sizeof(struct mm_struct));
    if (!mm) {
        printk("Failed to alloc for mm on kernel_exec\n");
        r = -1; goto out;
    }
    memset(mm, 0, sizeof(struct mm_struct));

    /* create pgd */
    new_pgd = mm_dup_pgd(kernel_pgd);
    if (new_pgd == 0) {
        printk("Failed to dup mm on kernel_exec\n");
        r = -1; goto out;
    }
    mm->pgd = new_pgd;
    current_task_TCB->mm = mm;

    if (load_elf(fs, filename, mm) != 0) {
        printk("Failed to load elf %s\n", filename);
        return -1;
    }

    r = 0;
    unlock_scheduler();
out:
    return r;
}