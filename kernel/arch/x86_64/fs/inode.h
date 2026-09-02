#ifndef _INODE_H
#define _INODE_H

#include <../include/defs.h>

struct inode_operations {
    struct dentry * (*lookup) (struct inode *, struct dentry *, unsigned int);
};

struct inode {
    struct inode_operations *i_op;
    unsigned long i_ino;
    off_t i_size;
};

#endif