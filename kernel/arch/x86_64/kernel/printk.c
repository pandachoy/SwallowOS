#include <stdarg.h>
#include <stdio.h>
#include "../cpu/cpu.h"

int printk(const char*restrict format, ...) {
    // lock_scheduler();
    int r;
    va_list parameters;
    va_start(parameters, format);
    r = vprintk_func(format, parameters);
    va_end(parameters);
    // unlock_scheduler();
    return r;
}

void panic(const char*restrict format, ...) {
    int r;
    // lock_scheduler();
    printk("[Error]");
    va_list parameters;
    va_start(parameters, format);
    r = vprintk_func(format, parameters);
    va_end(parameters);
    // unlock_scheduler();
    hlt();
    return r;
}