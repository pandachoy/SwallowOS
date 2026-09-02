#include "file.h"
#include "../sched/task.h"

#define FD_START                     3
#define MAX_OPEN_FILES           65535

int get_unused_fd() {
    static unsigned int fd_candidate = FD_START;
    if (fd_candidate >= MAX_OPEN_FILES) return -1;
    return fd_candidate++;
}

int sys_open(const char *filepath, int flags) {
    
}
