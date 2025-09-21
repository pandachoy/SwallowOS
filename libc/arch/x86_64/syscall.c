#include <stddef.h>
#include <stdint.h>
#include "../../include/syscall.h"

extern int libc_do_syscall(unsigned int nr,
                       void* arg1,
                       void* arg2, 
                       void* arg3,
                       void* arg4,
                       void* arg5,
                       void* arg6);

int read(int fd, size_t size, char *buffer) {
    return libc_do_syscall(0, fd, size, buffer, NULL, NULL, NULL);
}

int write(int fd, size_t size, char *buffer) {
    return libc_do_syscall(1, fd, size, buffer, NULL, NULL, NULL);
}

uint64_t get_task_id() {
    return libc_do_syscall(2, NULL, NULL, NULL, NULL, NULL, NULL);
}

uint64_t get_rsp0() {
    return libc_do_syscall(3, NULL, NULL, NULL, NULL, NULL, NULL);
}