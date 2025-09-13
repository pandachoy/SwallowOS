#ifndef _SYSCALL_H
#define _SYSCALL_H

int read(int fd, size_t size, char *buffer);

int write(int fd, size_t size, char *buffer);

#endif