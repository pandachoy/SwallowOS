#ifndef _ELFLOADER_H
#define _ELFLOADER_H

#include "fat.h"
#include "../mm/mm.h"

#define ELF_PAGESTART(_v) ((_v) & ~(PAGE_SIZE-1))
#define ELF_PAGEOFFSET(_v) ((_v) & (PAGE_SIZE-1))
#define ELF_PAGEALIGN(_v) (((_v) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

int load_elf(fat12_t *fs, const char *name, struct mm_struct *mm);

#endif