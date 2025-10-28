#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int memcmp(const void *a, const void *b, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *s, int c, size_t count);

#endif