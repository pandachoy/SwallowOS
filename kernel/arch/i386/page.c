
#include <stddef.h>
#include <kernel/page.h>     

/* 页分配有很多方法，如bitmap、stack/list、buddy alocations等，这里用最简单的bitmap */

#define MEM_END       (0xC0000000 + 1024 * 4096)               /* 暂时只讨论当前映射的一个页目录项 */
#define UINT32_BITS                           32
#define PRE_ALLOCATING_NUM                    20

/* 用来计算内核代码之后的 address of first page frame */
extern uint32_t _kernel_end;



uint32_t npages = 0;                        /* npages表示可分配的页个数，因为链接后才能得到 _kernel_end 的值，所以无法在编译期间计算，要运行之后计算 */
uint32_t *frame_map = NULL;                 /* frame_map标记某个页是否被使用，要放置在_kernel_end，同样也要运行之后计算 */
uint32_t *startframe = NULL;                /* 页帧起点，运行之后确定值 */

/* frame map operation */
static uint32_t get_frame_map(unsigned int index) {
    return (frame_map[index / UINT32_BITS] & (1 << (index % UINT32_BITS)))== 0 ? 0 : 1;
}
static void set_frame_map(unsigned int index, uint32_t val) {
    if (val > 0)
        frame_map[index/UINT32_BITS] |= 1 << (index % UINT32_BITS);
    else
        frame_map[index/UINT32_BITS] &= ~(1 << (index % UINT32_BITS));
}


// 函数 kalloc_frame_init 用于分配并初始化一个页面帧
pageframe_t kalloc_frame_init() {
    uint32_t i = 0;

    /* frame_map要sizeof(page_status)对齐 */
    if (!frame_map) {
        frame_map = (uint32_t *)(&_kernel_end + 1);
        /* 在frame_map后面填充其数据以及确定startframe */
        /* 首先要选一个合适的npages，设置为一个接近值x，则(x*4096 + x / 32) < (MEM_END -  frame_map)， 解出 x = (MEM_END - frame_map) / (4096 + 1/32), 实际上除数可以算成4097来估算 */
        npages = (MEM_END - (uint32_t)frame_map) / (0x1000 + 1);
        /* 得到npages，即可容易算出startframe，注意4k对齐 */
        startframe = frame_map + (npages / UINT32_BITS);
        if ((uint32_t)startframe % 0x1000 != 0)
            startframe = (uint32_t*)(((uint32_t)startframe / 0x1000 + 1) * 0x1000);
        /* 检查页面是否超出内存，注意这里要考虑到VGA video占用的页，所以在比较的时候要减去4096 */
        while (startframe + 0x1000 * npages > MEM_END - 0x1000)
            npages--;
        for (uint32_t *p = frame_map; p < startframe; ++p)
            *p = 0;
    }

    while(get_frame_map(i)) {
        i++;
        if (i == npages) {
            return NULL;
        }
    }
    set_frame_map(i, 1);
    return (startframe + (i * 0x1000)); /* 0x1000(4kb) */
}

static uint32_t *pre_frames[PRE_ALLOCATING_NUM] = {0};
pageframe_t kalloc_frame() {
    // static uint8_t allocate = 1;  /* whether or not we are going to allocate a new set of praframes */
    static uint8_t pframe = 0;    /* allocate pointer */
    pageframe_t ret;

    // if (pframe == PRE_ALLOCATING_NUM)
    //     allocate = 1;
    // if (allocate == 1) {
    //     for (uint32_t i = 0; i < PRE_ALLOCATING_NUM; ++i) {
    //         pre_frames[i] = kalloc_frame_init();
    //     }
    //     pframe = 0;
    //     allocate = 0;
    // }
    ret = kalloc_frame_init();

    return ret;
}

void kfree_frame(pageframe_t a) {
    if (a < startframe) return;

    unsigned int index = (a - startframe) / 0x1000;          /* get offset */
    set_frame_map(index, 0);
}