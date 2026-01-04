#include <stdio.h>
#include <syscall.h>

int main(int argc, char *argv[]) {
    int a = 3, b = 5, c; 
    c = c + b;
    for (unsigned int i = 0; i < 3; ++i) {
        printf("Hello world from app %u!\n", get_task_id());
        for(unsigned int i = 0; i < 1e8; ++i);
    }

    return 0;
}