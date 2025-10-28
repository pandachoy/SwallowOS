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
#include <kernel/malloc.h>
#include <kernel/semaphore.h>
#include <kernel/string.h>
#include <kernel/printk.h>
#include <kernel/pic.h>
#include "../arch/x86_64/pagemanager.h"
#include "../arch/x86_64/task.h"
#include "../arch/x86_64/cpu.h"
#include "../arch/x86_64/pgtable.h"
#include "../arch/x86_64/driver/floppy.h"
#include "../arch/x86_64/fs/fat.h"
#include "../multiboot/multiboot.h"

#include "../../../libc/include/syscall.h"

/* test helloworld */
void test_helloworld() {
    printk("Hello, %s!\nHi!", "World");
}

/* test keyboard */
extern volatile uint8_t key_buffer_pos;
extern volatile char keyboard_buffer[256];
void test_keyboard() {
    while(1) {
        if (key_buffer_pos > 0) {
            // for (int i = 0; i < key_buffer_pos; i++) {
            //     printk(keyboard_buffer[i]);
            // }
            printk("%s", keyboard_buffer);
            key_buffer_pos = 0;
        }
    }

}

/* test page allocation */
extern volatile uint64_t npages;                        /* npages表示可分配的页个数，因为链接后才能得到 _kernel_end 的值，所以无法在编译期间计算，要运行之后计算 */
extern volatile uint64_t *frame_map;                 /* frame_map标记某个页是否被使用，要放置在_kernel_end，同样也要运行之后计算 */
extern volatile uint64_t *startframe;                /* 页帧起点，运行之后确定值 */
void test_pagealloc() {
        pageframe_t page_array[200] = {0};
    unsigned int i = 0;

    int count = 0;
    for (i = 0; i < 200 ; ++i) {
        page_array[i] = kalloc_frame();
        if (!page_array[i]) {
            printk("No free pages left! %d\n", count);
            break;
        }
        count++;
    }

    printk("npages: %u\n", npages);
    printk("frame_map: %u\n", frame_map);
    printk("startframe: %u\n", startframe);

    kfree_frame(page_array[43]);
    kfree_frame(page_array[56]);
    kfree_frame(page_array[177]);
    count = 0;
    for (; i < 200; ++i) {
        page_array[i] = kalloc_frame();
        if (!page_array[i]) {
            printk("No free pages left! %d\n", count);
            break;
        }
        count++;
    }
}

/* test mm */
#define MEM_SIZE (1024*1024)
char MEM[MEM_SIZE] = { 0 };

#define M (16*16*16*16-1)
#define A 41649
#define C 31323
long long lcg(long long x) {
    return (A * x + C);
}
void test_mm(void) {
    long long seed = 2342;
    kmemory_init(MEM, MEM_SIZE);
    void *slots[1024] = {0};
    for (unsigned int i = 0; i < 1024; ++i) {
        seed = lcg(seed) % 10001;
        slots[i] = kmalloc(seed);
        if (!slots[i]) {
        printk("break at %u\n", i);
        break;
        }
    }

    printk("mm check: %d\n", kmcheck());
    for (unsigned int i = 0; i < 1024; ++i) {
        if (slots[i]) 
            kfree(slots[i]);
    }
}

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
    printk("(");
    if (l) {
        do {
            printk("%u ", (container_of(p, struct thread_control_block, tcb_list))->task_id);
            p = p->next;
        } while (p != l);
    }
    printk(") ");
}

static unsigned int list_size(struct list_head *list) {
    // printk("[inner %u]", list);
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
        printk("[%u ] ", current_task_TCB->task_id);
        // print_list(sleeping_task_list);
        // print_list(ready_tcb_list);
        // print_list(paused_task_list);
        print_list(smph->waiting_task_list);
        printk("smph->cur_count: %u ", smph->current_count);
        // printk("[outer %u]", ready_tcb_list);
        // list_size(ready_tcb_list);
        // printk("[after outer %u]", list_size(ready_tcb_list));
        // printk(pattern, str, current_task_TCB->task_id, list_size(ready_tcb_list), list_size(blocked_list));
        // lock_scheduler();
        // schedule();
        // unlock_scheduler();
          
        if (count%15==0) {
            static int a = 1;
            if (a == 1) {
                a = 0;
                printk("terminating task %u\n", current_task_TCB->task_id);
                terminate_task();
            }
        }
        else if (count%2==0 && paused_task_list) {
            printk("unblock ");
            unblock_task(container_of(paused_task_list, struct thread_control_block, tcb_list));
        }
        else if (count%7==0) {
            if (list_size(paused_task_list) <3) {
                printk("block ");
                block_task(PAUSED);                
            }
        }

        // else if (count%11==0) {
        //     if (postpone_flag) {
        //         postpone_flag = 0;
        //         printk("postpone ");
        //         lock_stuff();
        //     } else {
        //         postpone_flag = 1;
        //         printk("no postpone ");
        //         unlock_stuff();
        //     }
        // }
        else if (count%4==0) {
            printk("sleep %u ", current_task_TCB->task_id);
            nano_sleep_until(get_timer_count() + 65000);
        }
        else if (count%27==0) {
            printk("useless sleep %u ", current_task_TCB->task_id);
            nano_sleep_until(get_timer_count() - 1);
        }
        else {
            smph_count = (smph_count+1) % 10;
            if (smph_count < 5) {
                printk("acquire %u ", current_task_TCB->task_id);
                acquire_semaphore(smph);
            } else {
                printk("release %u ", current_task_TCB->task_id);
                release_semaphore(smph);
            }
        }

    }
}

/* test ring3 */
void print_hello() {
    printk("hello, ring3\n");
    for (unsigned int i=0; i<1e6; ++i);
}

void print_hello_ring0() {
    __asm__ volatile ("movl $0x10, %eax;"
                      "movw %ax, %ds;"
                      "movw %ax, %es;"
                      "movw %ax, %fs;"
                      "movw %ax, %gs");

    printk("hello, ring0\n");
    while(1);
}

void user_work() {
    while (1) {
        for (unsigned int i = 0; i < 2e8; ++i);
        uint64_t task_id = get_task_id();
        uint64_t rsp0 = get_rsp0();
        printk("hello, user work %u %u\n", task_id, rsp0);
    }
    
}

void user_work_wrapper() {
    get_to_ring3(user_work);
}

extern void do_syscall();

/* test physical memory */
extern volatile uint64_t npages;
extern void *p_multiboot_info;
void kernel_main(void);
void print_valid_memory(multiboot_info_t *mbd) {
    if(!(mbd->flags >> 6 & 0x1)) {
        printk("invalid memory map");
        return;
    }

    for(unsigned int i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t *mmmt = (multiboot_memory_map_t*) (mbd->mmap_addr + i);
        printk("Start Addr: %u | Length: %u | Size: %u | Type: %d\n",
            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);
        
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            printk("This mem map is available\n");
        }
    }
}

#include "../arch/x86_64/ram.h"
void test_pm(void) {
    printk("kernel main: %u\n", kernel_main);
    print_valid_memory(p_multiboot_info);

    kalloc_frame_init();
    printk("npages: %u\n", npages);
    printk("ram_start: %u, ram_end: %u\n", ram_start, ram_end);

    // printk("kernel_main physaddr: %u\n", get_physaddr(&page_map_level4, kernel_main));

}

/* test floppy disk */
void test_floppy_disk(void) {
    sti();
    IRQ_set_mask(0);
    floppy_init();
    char buffer[512] = {0};
    floppy_read_lba(1, buffer);
    printk("%s\n", buffer);
    buffer[1] = 'e';
    floppy_write_lba(1, buffer);
    for (unsigned int i=0; i<512; ++i) {
        buffer[i] = 0;
    }
    floppy_read_lba(1, buffer);
    printk("%s\n", buffer);
}

/* test fat */
void test_fat(void) {
    sti();
    IRQ_set_mask(0);
    floppy_init();

    fat12_t fs;
    printk("mount res: %d\n", fat12_mount(&fs));
    char buffer[4096] = {0};;
    uint32_t outlen;

    printk("read res: %d\n", fat12_read_file(&fs, "DREAM.TXT", buffer, 1024, &outlen));
    printk("file content:\n");
    printk("%s\n", buffer);
}

extern void *page_map_level4;
void kernel_main(void) {
    // load_gdt();
    terminal_initialize();
    PIC_init();
    // keyboard_init();
    load_idt();
    timer_init();
    NMI_enable();
    NMI_disable();

    test_fat();

    __asm__ volatile ("hlt");
}