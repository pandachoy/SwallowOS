#ifndef _CPU_H
#define _CPU_H

extern void get_to_ring3(void *user_func);
extern void get_to_ring0();
extern void set_ring0_msr(void *user_func);

#endif