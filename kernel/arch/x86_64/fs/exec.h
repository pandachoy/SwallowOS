#ifndef _EXEC_H
#define _EXEC_H

#include "elfloader.h"

int kernel_exec(fat12_t *fs, const char *filename, const char *const *argv, const char *const *envp);

#endif