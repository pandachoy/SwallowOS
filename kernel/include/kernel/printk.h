#ifndef PRINTK_H
#define PRINTK_H

int printk(const char* __restrict, ...);
void panic(const char*restrict format, ...);

#endif