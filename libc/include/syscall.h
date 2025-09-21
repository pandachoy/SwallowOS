#ifndef _SYSCALL_H
#define _SYSCALL_H

int read(int fd, size_t size, char *buffer);

int write(int fd, size_t size, char *buffer);

uint64_t get_task_id();

void exit();

uint64_t get_rsp0();

#endif