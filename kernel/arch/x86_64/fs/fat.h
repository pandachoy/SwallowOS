#ifndef FAT_H
#define FAT_H

#define BYTES_PER_SECTOR             512

typedef struct fat_BS {
    unsigned char    bootjmp[3];
    unsigned char    oem_name[8];
    unsigned short   bytes_per_sector;
    unsigned char    sectors_per_cluster;
    unsigned short   reserved_sector_count;
    unsigned char    table_count;
    unsigned short   root_entry_count;
    unsigned short   total_sectors_16;
    unsigned char    media_type;
    unsigned short   table_size_16;
    unsigned short   sectors_per_track;
    unsigned short   head_side_count;
    unsigned int     hidden_sector_count;
    unsigned int     total_sectors_32;

    unsigned char    extended_section[476];
} __attribute__((packed)) fat_BS_t;

typedef struct fat_extBS_16 {
    unsigned char    bios_drive_num;
    unsigned char    reserved1;
    unsigned char    signature;
    unsigned int     volume_id;
    unsigned char    volume_label[11];
    unsigned char    fat_type_label[8];
    unsigned char    boot_code[448];
    unsigned short   boot_signature;
} __attribute__((packed)) fat_extBS_16_t;

typedef struct fat_extBS_32 {
    unsigned int     table_size_32;
    unsigned short   extended_flags;
    unsigned short   fat_version;
    unsigned int     root_cluster;
    unsigned short   fat_info;
    unsigned short   backup_BS_sector;
    unsigned char    reserved_0[12];
    unsigned char    drive_number;
    unsigned char    reserved_1;
    unsigned char    signature;
    unsigned int     volume_id;
    unsigned char    volume_label[11];
    unsigned char    fat_type_label[8];
    unsigned char    boot_code[420];
    unsigned short   boot_signature;
} __attribute__((packed)) fat_extBS_32_t;

typedef struct fat12 {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors;
    uint16_t sectors_per_fat;
    uint32_t fat_start_sector;
    uint32_t root_start_lba;
    uint32_t data_start_lba;
    uint32_t root_dir_sectors;
} fat12_t;

bool fat12_mount(fat12_t *fs);
bool fat12_read_file(fat12_t *fs, const char* name83, uint8_t *out, uint32_t max_len, uint32_t *outlen);
bool fat12_write_file(fat12_t *fs, const char *name83, const uint8_t *data, uint32_t len);

#endif