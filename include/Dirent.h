#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <fat.h>

#define DIRENT_NUM 4096
typedef struct Dirent {
    char filename[FAT32_MAX_FILENAME + 1];
    uint8 attribute;
    // uint8   create_time_tenth;
    // uint16  create_time;
    // uint16  create_date;
    // uint16  last_access_date;
    uint32 first_clus;
    // uint16  last_write_time;
    // uint16  last_write_date;
    uint32 file_size;

    uint32 cur_clus;
    u32 inodeMaxCluster;
    uint clus_cnt;
    Inode inode;

    u8 _nt_res;
    FileSystem *fileSystem;
    /* for OS */
    enum { ZERO = 10, OSRELEASE=12, NONE=15 } dev;
    FileSystem *head;
    uint32 off;  // offset in the parent dir entry, for writing convenience
    struct Dirent* parent;  // because FAT32 doesn't have such thing like inum,
    struct Dirent* nextBrother;
    struct Dirent* firstChild;
} Dirent;

typedef struct short_name_entry {
    char name[CHAR_SHORT_NAME];
    uint8 attr;
    uint8 _nt_res;
    uint8 _crt_time_tenth; // 39-32 of access
    uint16 _crt_time; // 31-16 of access
    uint16 _crt_date; // 15-0 of access
    uint16 _lst_acce_date; // 47-32 of modify
    uint16 fst_clus_hi;
    uint16 _lst_wrt_time; // 31-16 of modify
    uint16 _lst_wrt_date; // 15-0 of modify
    uint16 fst_clus_lo;
    uint32 file_size;
} __attribute__((packed, aligned(4))) short_name_entry_t;

typedef struct long_name_entry {
    uint8 order;
    wchar name1[5];
    uint8 attr;
    uint8 _type;
    uint8 checksum;
    wchar name2[6];
    uint16 _fst_clus_lo;
    wchar name3[2];
} __attribute__((packed, aligned(4))) long_name_entry_t;

union dentry {
    short_name_entry_t sne;
    long_name_entry_t lne;
};

int direntAlloc(Dirent **d);
void direntFree(Dirent *d);
void direntInit();

#endif