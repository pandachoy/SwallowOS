#ifndef _FILE_H
#define _FILE_H

#include <kernel/list.h>
#include <../include/defs.h>
#include "inode.h"

#define DNAME_INLINE_LEN           32

struct dentry {
    struct dentry *d_parent;
    char d_iname[DNAME_INLINE_LEN];
};



struct file_operations {
    int (*open)(struct inode *, struct file *);
    ssize_t (*read)(struct file *, char *, size_t, off_t *);
    ssize_t (*write)(struct file *, const char *, size_t, off_t *);
};

struct file {
    struct inode *f_inode;
    struct file_operations *f_op;
};

struct file_struct {

};

int sys_open(const char *filepath, int flags);

#endif