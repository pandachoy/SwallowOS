#include <stdio.h>
#include <syscall.h>

int main(int argc, char *argv[]) {
    int a = 3, b = 5, c; 
    c = c + b;
    for (unsigned int i = 0; i < 10; ++i) {
        // { volatile unsigned long _i; for(_i=0; _i<50000000; ++_i); }
    printf("Hello world from app %u!\n", get_task_id());
        for(unsigned int i = 0; i < 1e8; ++i);
    }

    return 0;
}