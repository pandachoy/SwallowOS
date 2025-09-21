#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/pic.h>
#include <kernel/nmi.h>
#include <kernel/keyboard.h>
#include <kernel/timer.h>
#include <kernel/page.h>
#include <kernel/list.h>
#include <kernel/mm.h>
#include <kernel/semaphore.h>
#include "../arch/x86_64/pagemanager.h"
#include "../arch/x86_64/task.h"
#include "../arch/x86_64/cpu.h"

#include "../../../libc/include/syscall.h"

/* test helloworld */
void test_helloworld() {
    printf("Hello, %s!\nHi!", "World");
}

/* test keyboard */
// extern volatile uint8_t key_buffer_pos;
// extern volatile char keyboard_buffer[256];
// void test_keyboard() {
//     while(1) {
//         if (key_buffer_pos > 0) {
//             // for (int i = 0; i < key_buffer_pos; i++) {
//             //     printf(keyboard_buffer[i]);
//             // }
//             printf("%s", keyboard_buffer);
//             key_buffer_pos = 0;
//         }
//     }

// }

/* test page allocation */
// extern volatile uint32_t npages;                        /* npages表示可分配的页个数，因为链接后才能得到 _kernel_end 的值，所以无法在编译期间计算，要运行之后计算 */
// extern volatile uint32_t *frame_map;                 /* frame_map标记某个页是否被使用，要放置在_kernel_end，同样也要运行之后计算 */
// extern volatile uint32_t *startframe;                /* 页帧起点，运行之后确定值 */
// void test_pagealloc() {
//         pageframe_t page_array[200] = {0};
//     unsigned int i = 0;

//     int count = 0;
//     for (i = 0; i < 200 ; ++i) {
//         page_array[i] = kalloc_frame();
//         if (!page_array[i]) {
//             printf("No free pages left! %d\n", count);
//             break;
//         }
//         count++;
//     }

//     printf("npages: %u\n", npages);
//     printf("frame_map: %u\n", frame_map);
//     printf("startframe: %u\n", startframe);

//     kfree_frame(page_array[43]);
//     kfree_frame(page_array[56]);
//     kfree_frame(page_array[177]);
//     count = 0;
//     for (; i < 200; ++i) {
//         page_array[i] = kalloc_frame();
//         if (!page_array[i]) {
//             printf("No free pages left! %d\n", count);
//             break;
//         }
//         count++;
//     }
// }

/* test mm */
// #define MEM_SIZE (1024*1024)
// char MEM[MEM_SIZE] = { 0 };

// #define M (16*16*16*16-1)
// #define A 41649
// #define C 31323
// long long lcg(long long x) {
//     return (A * x + C);
// }
// void test_mm(void) {
//     long long seed = 2342;
//     kmemory_init(MEM, MEM_SIZE);
//     void *slots[1024] = {0};
//     for (unsigned int i = 0; i < 1024; ++i) {
//         seed = lcg(seed) % 10001;
//         slots[i] = kmalloc(seed);
//         if (!slots[i]) {
//         printf("break at %u\n", i);
//         break;
//         }
//     }

//     printf("mm check: %d\n", kmcheck());
//     for (unsigned int i = 0; i < 1024; ++i) {
//         if (slots[i]) 
//             kfree(slots[i]);
//     }
// }

/* test task */
unsigned char ch_index = 0;

extern int irq_disable_counter;
extern int postpone_task_switches_counter;
extern struct list_head *ready_tcb_list;
extern struct list_head *paused_task_list;
extern struct list_head *sleeping_task_list;
extern unsigned long time_slice_remaining;

int block_status = 1;
extern unsigned long get_timer_count();

void print_list(struct list_head *l) {
    struct list_head *p = l;
    printf("(");
    if (l) {
        do {
            printf("%u ", (container_of(p, struct thread_control_block, tcb_list))->task_id);
            p = p->next;
        } while (p != l);
    }
    printf(") ");
}

static unsigned int list_size(struct list_head *list) {
    // printf("[inner %u]", list);
    if (!list)
        return 0;

    unsigned int size = 1;
    struct list_head *p = list->next;
    while (p != list) {
        size++;
        p = p->next;
    }
    return size;
}

struct semaphore *smph = NULL;
unsigned int smph_count = 0;

void test_task(void) {
    char str[2] = { 0 };
    str[0] = 'A'; // + ch_index;
    ch_index = (ch_index + 1) % 26;
    static unsigned long count = UINT64_MAX;

    static unsigned long postpone_flag = 1;
    while (count--) {
        for (unsigned long i=0; i<500000000; ++i);
        if (current_task_TCB->state != RUNNING)
            str[0] = 'a'; //str[0] - 'A' + 'a';
        else
            str[0] = 'A';
        char *pattern = "[%s %u %u %u] ";
        printf("[%u ] ", current_task_TCB->task_id);
        // print_list(sleeping_task_list);
        // print_list(ready_tcb_list);
        // print_list(paused_task_list);
        print_list(smph->waiting_task_list);
        printf("smph->cur_count: %u ", smph->current_count);
        // printf("[outer %u]", ready_tcb_list);
        // list_size(ready_tcb_list);
        // printf("[after outer %u]", list_size(ready_tcb_list));
        // printf(pattern, str, current_task_TCB->task_id, list_size(ready_tcb_list), list_size(blocked_list));
        // lock_scheduler();
        // schedule();
        // unlock_scheduler();
          
        if (count%15==0) {
            static int a = 1;
            if (a == 1) {
                a = 0;
                printf("terminating task %u\n", current_task_TCB->task_id);
                terminate_task();
            }
        }
        else if (count%2==0 && paused_task_list) {
            printf("unblock ");
            unblock_task(container_of(paused_task_list, struct thread_control_block, tcb_list));
        }
        else if (count%7==0) {
            if (list_size(paused_task_list) <3) {
                printf("block ");
                block_task(PAUSED);                
            }
        }

        // else if (count%11==0) {
        //     if (postpone_flag) {
        //         postpone_flag = 0;
        //         printf("postpone ");
        //         lock_stuff();
        //     } else {
        //         postpone_flag = 1;
        //         printf("no postpone ");
        //         unlock_stuff();
        //     }
        // }
        else if (count%4==0) {
            printf("sleep %u ", current_task_TCB->task_id);
            nano_sleep_until(get_timer_count() + 65000);
        }
        else if (count%27==0) {
            printf("useless sleep %u ", current_task_TCB->task_id);
            nano_sleep_until(get_timer_count() - 1);
        }
        else {
            smph_count = (smph_count+1) % 10;
            if (smph_count < 5) {
                printf("acquire %u ", current_task_TCB->task_id);
                acquire_semaphore(smph);
            } else {
                printf("release %u ", current_task_TCB->task_id);
                release_semaphore(smph);
            }
        }

    }
}

/* test ring3 */
void print_hello() {
    printf("hello, ring3\n");
    for (unsigned int i=0; i<1e6; ++i);
}
void print_hello_ring0() {
    __asm__ volatile ("movl $0x10, %eax;"
                      "movw %ax, %ds;"
                      "movw %ax, %es;"
                      "movw %ax, %fs;"
                      "movw %ax, %gs");
    printf("hello, ring0\n");
    while(1);
}

void user_work() {
    while (1) {
        for (unsigned int i = 0; i < 2e8; ++i);
        uint64_t task_id = get_task_id();
        uint64_t rsp0 = get_rsp0();
        printf("hello, user work %u %u\n", task_id, rsp0);
    }
    
}

void user_work_wrapper() {
    get_to_ring3(user_work);
}
extern void do_syscall();

void kernel_main(void) {
    // load_gdt();
    terminal_initialize();
    PIC_init();
    // keyboard_init();
    load_idt();
    timer_init();
    NMI_enable();
    NMI_disable();

    /* set ring0 msr */
    set_ring0_msr(do_syscall);
    init_scheduler();
    for (unsigned int i = 0; i < 3; ++i) {
        struct thread_control_block *tcb = create_task(user_work_wrapper);
    }
    sti();
    kernel_idle_work();
    

    __asm__ volatile ("hlt");
}