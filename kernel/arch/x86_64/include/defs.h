#ifndef _DEFS_H
#define _DEFS_H

#define offset_of(type, member) \
    (uint64_t)&(((type *)0)->member)

typedef long unsigned int size_t;
typedef long int ssize_t;
typedef long long off_t;

#endif