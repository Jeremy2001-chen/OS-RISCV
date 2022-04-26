#ifndef __FAT32_H
#define __FAT32_H

#include "Sleeplock.h"
#include "Type.h"
#include "stat.h"

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F

#define LAST_LONG_ENTRY 0x40
#define FAT32_EOC 0x0ffffff8
#define EMPTY_ENTRY 0xe5
#define END_OF_ENTRY 0x00
#define CHAR_LONG_NAME 13
#define CHAR_SHORT_NAME 11

#define FAT32_MAX_FILENAME 255
#define FAT32_MAX_PATH 260
#define ENTRY_CACHE_NUM 50

struct superblock {
    uint32 first_data_sec;
    uint32 data_sec_cnt;
    uint32 data_clus_cnt;
    uint32 byts_per_clus;

    struct {
        uint16 byts_per_sec;
        uint8 sec_per_clus;
        uint16 rsvd_sec_cnt;
        uint8 fat_cnt;   /* count of FAT regions */
        uint32 hidd_sec; /* count of hidden sectors */
        uint32 tot_sec;  /* total count of sectors including all regions */
        uint32 fat_sz;   /* count of sectors for a FAT region */
        uint32 root_clus;
    } bpb;
};

struct dirent {
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
    uint clus_cnt;

    /* for OS */
    uint8 dev;
    uint8 dirty;
    short valid;
    int ref;
    uint32 off;  // offset in the parent dir entry, for writing convenience
    struct dirent* parent;  // because FAT32 doesn't have such thing like inum,
                            // use this for cache trick
    struct dirent* next;
    struct dirent* prev;
    struct Sleeplock lock;
};

int fat32_init(void);
struct dirent* dirlookup(struct dirent* entry, char* filename, uint* poff);
char* formatname(char* name);
void emake(struct dirent* dp, struct dirent* ep, uint off);
struct dirent* ealloc(struct dirent* dp, char* name, int attr);
struct dirent* edup(struct dirent* entry);
void eupdate(struct dirent* entry);
void etrunc(struct dirent* entry);
void eremove(struct dirent* entry);
void eput(struct dirent* entry);
void estat(struct dirent* ep, struct stat* st);
void elock(struct dirent* entry);
void eunlock(struct dirent* entry);
int enext(struct dirent* dp, struct dirent* ep, uint off, int* count);
struct dirent* ename(char* path);
struct dirent* enameparent(char* path, char* name);
int eread(struct dirent* entry, int user_dst, u64 dst, uint off, uint n);
int ewrite(struct dirent* entry, int user_src, u64 src, uint off, uint n);
struct dirent* create(char* path, short type, int mode);
#endif