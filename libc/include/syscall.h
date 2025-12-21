#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <stdint.h>
#include <stddef.h>

int read(int fd, size_t size, char *buffer);

int write(int fd, size_t size, char *buffer);

uint64_t get_task_id();

int exit(int code);

uint64_t get_rsp0();

int putchar(int ic);

#endif