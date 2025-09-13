#include <stddef.h>
#include <stdio.h>

int sys_read(int fd, size_t size, char *buffer) {
    printf("read called %d\n", (int)buffer);
    return 1;
}

int sys_write(int fd, size_t size, char *buffer) {
    printf("write called %d\n", (int)buffer);
    return 2;
}

void * syscalls[] = {
    sys_read,
    sys_write
};

