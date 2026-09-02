#include <stdio.h>
#include <syscall.h>

void print_title() {
    printf("Swallow:");
}

int main(int argc, char *argv[]) {
    // for (unsigned int i = 0; i < 3; ++i) {
    //     printf("Shell from app %u!\n", get_task_id());
    //     for(unsigned int i = 0; i < 1e8; ++i);
    // }
    
    print_title();
    while (1) {
        int ch = getchar();
        if (ch >= 0) {
            putchar(ch);
            if (ch == '\n')
                print_title();
        }
    }
    return 0;
}