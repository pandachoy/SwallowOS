#include <kernel/string.h>

int memcmp(const void *a, const void *b, size_t count) {
    const unsigned char *su1, *su2;
    int res = 0;

    for (su1 = a, su2 = b; count > 0; ++su1, ++su2, --count) {
        if ((res = *su1 - *su2) != 0)
            break;
    }
    return res;
}

void *memcpy(void *dest, const void *src, size_t count) {
    char *tmp = dest;
    const char *s = src;

    while (count--) {
        *tmp++ = *s++
    }
    return dest;
}

void *memset(void *s, int c, size_t count) {
    char *xs = s;

    while (count--) {
        *xs++ = c;
    }
    return s;
}