#include <stdio.h>
#include <kernel/string.h>
#include <kernel/printk.h>
#include "../driver/floppy.h"
#include "fat.h"

#define BYTES_PER_ROOT_DIR            32

/* directory struct */
typedef struct __attribute__((packed)) {
    char name[8];
    char ext[3];
    uint8_t attr;
    uint8_t reserved;
    uint8_t creat_time_in_hundreds;
    uint16_t creat_time;
    uint16_t creat_date;
    uint16_t access_date;
    uint16_t first_cluster_number_high;             /* always zero in fat12 and fat16 */
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t first_cluster_number_low;
    uint32_t size;
} dirent_t;

static bool read_sector(uint32_t lba, uint8_t *buf) {
    return floppy_read_lba(lba, buf);
}

static uint32_t cluster_to_lba(const fat12_t *fs, uint16_t cluster) {
    return fs->data_start_lba + (uint32_t)(cluster - 2) * fs->sectors_per_cluster;        
}

static bool read_fat_entry(const fat12_t *fs, uint16_t cluster, uint16_t *value) {
    uint32_t fat_offset = cluster + (cluster / 2);
    uint32_t fat_sector = fs->fat_start_sector + (fat_offset / BYTES_PER_SECTOR);
    uint32_t ent_offset = fat_offset % BYTES_PER_SECTOR;
    uint8_t sec[BYTES_PER_SECTOR] = {0};
    if (!read_sector(fat_sector, sec))
        return false;
    uint16_t val = *(uint16_t*)&sec[ent_offset];
    /* pick 12 bits */
    if (cluster & 1)       /* odd */
        val = val >> 4;
    else                   /* even */
        val &= 0x0FFF;
    *value = val;
    return true;
}

bool fat12_mount(fat12_t *fs) {
    if (!fs) return false;

    char buffer[BYTES_PER_SECTOR];
    if (!read_sector(0, buffer)) return false;

    fat_BS_t *fat = (fat_BS_t*)buffer;

    /* check fat */
    if (fat->bytes_per_sector != BYTES_PER_SECTOR) {
        printk("Invalid bytes per sector\n");
        return false;
    }

    fat_extBS_16_t *fat12 = fat->extended_section;
    if (fat12->boot_signature != 0xAA55) {
        printk("Invalid signature of fat12\n");
        return false;
    }
    if (fat->table_size_16 == 0) {
        printk("No table size 16\n");
        return false;
    }

    fs->bytes_per_sector = fat->bytes_per_sector;
    fs->sectors_per_cluster = fat->sectors_per_cluster;
    fs->reserved_sectors = fat->reserved_sector_count;
    fs->num_fats = fat->table_count;
    fs->root_entry_count = fat->root_entry_count;
    fs->total_sectors = fat->total_sectors_16 != 0 ? fat->total_sectors_16 : fat->total_sectors_32;
    fs->sectors_per_fat = fat->table_size_16;
    fs->fat_start_sector = fs->reserved_sectors;
    fs->root_start_lba = fs->fat_start_sector + fs->num_fats * fs->sectors_per_fat;
    fs->root_dir_sectors = ((fs->root_entry_count * BYTES_PER_ROOT_DIR) + (fs->bytes_per_sector - 1)) / fs->bytes_per_sector;
    fs->data_start_lba = fs->root_start_lba + fs->root_dir_sectors;

    printk("bytes_per_sector: %u\n", fs->bytes_per_sector);
    printk("sectors_per_cluster: %u\n", fs->sectors_per_cluster);
    printk("reserved_sectors: %u\n", fs->reserved_sectors);
    printk("num_fats: %u\n", fs->num_fats);
    printk("root_dir_sectors: %u\n", fs->root_dir_sectors);
    printk("total_sectors: %u\n", fs->total_sectors);
    printk("sectors_per_fat: %u\n", fs->sectors_per_fat);
    printk("fat_start_sector: %u\n", fs->fat_start_sector);
    printk("root_start_lba: %u\n", fs->root_start_lba);
    printk("root_dir_sectors: %u\n", fs->root_dir_sectors);
    printk("data_start_lba: %u\n", fs->data_start_lba);
    return true;
}

static void upper_padding(const char *in, char *name8, char *ext3) {
    for (unsigned int i=0; i<8; ++i)
        name8[i] = ' ';
    for (unsigned int i=0; i<3; ++i)
        ext3[i] = ' ';
    
    unsigned int i=0;
    while (in[i] && in[i] != '.' && i < 8) {
        char c = in[i];
        if (c>='a' && c<='z')
            c -= 32;
        name8[i] = c;
        ++i;
    }
    if (in[i] == '.') {
        ++i;
        int j = 0;
        while (in[i] && j<3) {
            char c = in[i];
            if (c>='a' && c<='z')
                c -= 32;
            ext3[j] = c;
            ++i;
            ++j;
        }
    }   
}

bool fat12_read_file(fat12_t *fs, const char* name83, uint8_t *out, uint32_t max_len, uint32_t *outlen) {
    char n8[8];     /* basename */
    char e3[3];     /* extension */
    upper_padding(name83, n8, e3);
    uint8_t sec[BYTES_PER_SECTOR] = {0};
    /* traverse root directories */
    for (uint32_t i=0; i<fs->root_dir_sectors; ++i) {
        if (!read_sector(fs->root_start_lba + i, sec)) return false;
        dirent_t *ents = (dirent_t*)sec;
        for (unsigned int j=0; j<BYTES_PER_SECTOR/BYTES_PER_ROOT_DIR; ++j) {
            if (ents[j].name[0] == 0x00) return false;
            if ((uint8_t)ents[j].name[0] == 0xE5) continue;           /* unused */
            if ((ents[j].attr & 0x0F) == 0x0F) continue;              /* long file name, not supported here */
            if (memcmp((uint8_t*)ents[j].name, (uint8_t*)n8, 8) == 0 && memcmp((uint8_t*)ents[j].ext, (uint8_t*)e3, 3) == 0) {      /* file found */
                uint32_t remaining = ents[j].size;
                uint16_t cl = ents[j].first_cluster_number_low;
                uint32_t pos = 0;
                while (cl >= 2 && cl < 0xFF8 && remaining > 0) {      /* 0xFF8 means no more clusters in the chain*/
                    if (pos >= max_len) break;
                    uint32_t lba = cluster_to_lba(fs, cl);            /* lba */
                    memset(sec, 0, BYTES_PER_SECTOR);
                    if (!read_sector(lba, sec))
                        return false;
                    uint32_t tocpy = remaining > BYTES_PER_SECTOR ? BYTES_PER_SECTOR : remaining;
                    if (pos + tocpy > max_len)
                        tocpy = max_len - pos;
                    memcpy(out + pos, sec, tocpy);
                    pos += tocpy;
                    remaining -= tocpy;
                    if (!read_fat_entry(fs, cl, &cl))                  /* cluster chain */
                        return false;
                }
                if (outlen)
                    *outlen = pos;
                return true;
            }
        }
    }
    return false;
}


bool fat12_write_file(fat12_t *fs, const char *name83, const uint8_t *data, uint32_t len) {
    return true;
}