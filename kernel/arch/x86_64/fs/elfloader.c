#include <stdbool.h>
#include "fat.h"
#include <kernel/malloc.h>
#include <kernel/printk.h>
#include <string.h>
#include <kernel/page.h>
#include "../sched/task.h"
#include "../mm/mm.h"
#include "../mm/pagemanager.h"
#include "elf.h"
#include "elfloader.h"

static void read_elf(const char *elf_content, unsigned int len) {
    if (len == 0) {
        printk("Invalid elf content\n");
        return;
    }

    printk("Read ELF\n");

    /* check elf magic code */
    Elf64_Ehdr *elf_hdr = (Elf64_Ehdr* )elf_content;
    printk("    ELF Header: %s\n", &(elf_hdr->e_ident[0]));

    /* elf type */
    char *type = "NONE";
    switch (elf_hdr->e_type) {
        case ET_EXEC:
            type = "EXEC";
            break;
        case ET_DYN:
            type = "DYN";
            break;
        case ET_REL:
            type = "REL";
            break;
        case ET_NONE:
            type = "NONE";
            break;
        default:
            type = "OTHER";
            break;
    }
    printk("    ELF type: %s\n", type);

    /* elf machine */
    char *machine = "NONE";
    switch (elf_hdr->e_machine) {
        case EM_X86_64:
            machine = "x86_64";
            break;
        case EM_NONE:
            machine = "NONE";
            break;
        default:
            machine = "OTHER";
            break;
    }
    printk("    ELF machine: %s\n", machine);

    /* elf version */
    printk("    ELF version: %d\n", (elf_hdr->e_version & 0b11));

    /* elf program header */
    printk("    ELF header:\n");
    Elf64_Phdr *p_hdr = elf_content + elf_hdr->e_phoff;
    for (unsigned int i=0; i<elf_hdr->e_phnum; ++i) {
        /* program header type */
        char *p_type = "NULL";
        switch (p_hdr[i].p_type) {
            case PT_LOAD:
                p_type = "LOAD   ";
                break;
            case PT_DYNAMIC:
                p_type = "DYNAMIC";
                break;
            case PT_INTERP:
                p_type = "INTERP ";
                break;
            case PT_NOTE:
                p_type = "NOTE   ";
                break;
            case PT_PHDR:
                p_type = "PHDR   ";
                break;
            case PT_NUM:
                p_type = "NUM    ";
                break;
            case PT_NULL:
                p_type = "NULL   ";
                break;
            default:
                p_type = "OTHER  ";
                break;
        }
        printk("     Type: %s", p_type);

        /* program header flags */
        char p_flags[4] = {'_', '_', '_', '\0'};
        if (p_hdr[i].p_flags & PF_X)
            p_flags[0] = 'X';
        if (p_hdr[i].p_flags & PF_W)
            p_flags[1] = 'W';
        if (p_hdr[i].p_flags & PF_R)
            p_flags[2] = 'R';
        printk("  Flags: %s", &(p_flags[0]));

        /* program header alignment */
        printk("  Align: %u", p_hdr[i].p_align);

        /* program header offset */
        printk("  Offset: %x\n", p_hdr[i].p_offset);

        /* program header vaddr */
        printk("      Vaddr: %x", p_hdr[i].p_vaddr);

        /* program header paddr */
        printk("  Paddr: %x", p_hdr[i].p_paddr);

        /* program header file size */
        printk("  Filesz: %x", p_hdr[i].p_filesz);

        /* program header memory size */
        printk("  Memsz: %x", p_hdr[i].p_memsz);

        printk("\n");
    }

    return;
}

static int _do_load_elf(char *elf_content, struct mm_struct *mm) {
    Elf64_Ehdr *elf_hdr = (Elf64_Ehdr* )elf_content;

    /* check magic */
    if (elf_hdr->e_ident[EI_MAG0] != ELFMAG0
     || elf_hdr->e_ident[EI_MAG1] != ELFMAG1
     || elf_hdr->e_ident[EI_MAG2] != ELFMAG2
     || elf_hdr->e_ident[EI_MAG3] != ELFMAG3) {
        printk("[Error] Not a valid elf\n");
        return -1;
    }

    /* check type */
    if (elf_hdr->e_type !=  ET_EXEC) {
    printk("[Error] ELF not EXEC type\n");
    return -1;
    }

    /* check machine */
    if (elf_hdr->e_machine != EM_X86_64) {
    printk("[Error] ELF not of x86_64 machine\n");
    return -1;
    }

    /* load elf program header */
    Elf64_Phdr *p_hdr = elf_content + elf_hdr->e_phoff;
    for (unsigned int i=0; i<elf_hdr->e_phnum; ++i) {
        if (p_hdr[i].p_type != PT_LOAD)
            continue;

        struct vm_area_struct *vma = (struct vm_area_struct *)kmalloc(sizeof(struct vm_area_struct));
        if (!vma) {
            printk("[Error] Failed to alloc space of vma\n");
            return -1;
        }
        vma->vm_flags = p_hdr[i].p_type;
        vma->vm_start = p_hdr[i].p_vaddr;
        vma->vm_end = p_hdr[i].p_vaddr + p_hdr[i].p_memsz;
        vma->vm_pgoff = p_hdr[i].p_offset;
        vma->vm_mm = mm;

        if (!mm->mmap) {
            mm->mmap = vma;
            INIT_LIST_HEAD(&mm->mmap->vma_list);
        } else {
            list_add_tail(&vma->vma_list, &mm->mmap->vma_list);
        }
    }

    /* add stack, use constants for now */
    struct vm_area_struct *vma = (struct vm_area_struct *)kmalloc(sizeof(struct vm_area_struct));
    unsigned int stack_page_num = 23;
    vma->vm_flags = 0;
    vma->vm_start = 0x00007ffffffdc000;
    vma->vm_end = vma->vm_start + stack_page_num * PAGE_SIZE;
    vma->vm_pgoff = vma->vm_end - vma->vm_start;
    vma->vm_mm = mm;
    if (!mm->mmap) {
        mm->mmap = vma;
        INIT_LIST_HEAD(&mm->mmap->vma_list);
    } else {
        list_add_tail(&vma->vma_list, &mm->mmap->vma_list);
    }
    /* alloc user stack */
    for (uint64_t pg_index = 0; pg_index < stack_page_num; ++pg_index) {
        do_mmap((uint64_t)vma->vm_start + pg_index * PAGE_SIZE, mm);
    }
    mm->rsp = vma->vm_end;
    setcr3(mm->pgd);

    mm->elf_content = elf_content;

    unlock_scheduler();  /* matches lock_scheduler in kernel_exec */
    get_to_ring3(elf_hdr->e_entry);
    // printk("e_entry: %x\n", elf_hdr->e_entry);

    return 0;
}

int load_elf(fat12_t *fs, const char *name, struct mm_struct *mm) {
    int r = 0;

    uint8_t sec[BYTES_PER_SECTOR] = {0};
    int64_t size = fat12_get_file_size(fs, name);
    if (size <= 0) {
        printk("[Error] Failed to get file size of %s\n", name);
        r = -1;
        goto out;
    }
    // printk("Get elf size: %u\n", size);

    uint8_t *file_content = (uint8_t *)kmalloc(size + 1);
    if (!file_content) {
        printk("[Error] Failed to alloc file buffer of %s\n", name);
        r = -1;
        goto out;
    }
    memset(file_content, '\0', size);

    unsigned int outlen;
    if (!fat12_read_file(fs, name, file_content, size, &outlen)) {
        printk("[Error] Failed to read file %s\n", name);
        kfree(file_content);
        r = -1;
        goto out;
    }

    // read_elf(file_content, outlen);

    if (_do_load_elf(file_content, mm) != 0) {
        printk("[Error] Failed to load elf content\n");
        kfree(file_content);
        r = -1;
        goto out;
    }

    // theoretically it never reach here
    panic("If kernel reach here, it is in trouble\n");
    r = 0;
out:
    return r;
}

