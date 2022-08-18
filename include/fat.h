#ifndef __FAT32_H
#define __FAT32_H

#include "Sleeplock.h"
#include "Type.h"
#include "stat.h"
#include "Timer.h"
#include "Inode.h"

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F
#define ATTR_LINK   0x40
#define ATTR_CHARACTER_DEVICE 0x80

#define LAST_LONG_ENTRY 0x40
#define FAT32_EOC 0x0ffffff8
#define EMPTY_ENTRY 0xe5
#define END_OF_ENTRY 0x00
#define CHAR_LONG_NAME 13
#define CHAR_SHORT_NAME 11

#define FAT32_MAX_FILENAME 255
#define FAT32_MAX_PATH 260

typedef struct FileSystem FileSystem;
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

typedef struct Dirent Dirent;

struct linux_dirent64 {
    u64 d_ino;               /* 64-bit inode number */
    u64 d_off;               /* 64-bit offset to next structure */
    unsigned short d_reclen; /* Size of this dirent */
    unsigned char d_type;    /* File type */
    char d_name[];           /* Filename (null-terminated) */
};

int fat32_init(void);
Dirent* dirlookup(Dirent* entry, char* filename);
char* formatname(char* name);
int getBlockNumber(Dirent* entry, int dataBlockNum);
void emake(Dirent* dp, Dirent* ep, uint off);
Dirent* ealloc(Dirent* dp, char* name, int attr);
Dirent* edup(Dirent* entry);
void eupdate(Dirent* entry);
void etrunc(Dirent* entry);
void eremove(Dirent* entry);
void eput(Dirent* entry);
void estat(Dirent* ep, struct stat* st);
void elock(Dirent* entry);
void eunlock(Dirent* entry);
Dirent* ename(int fd, char* path, bool jump);
Dirent* enameparent(int fd, char* path, char* name);
int eread(Dirent* entry, int user_dst, u64 dst, uint off, uint n);
int ewrite(Dirent* entry, int user_src, u64 src, uint off, uint n);
Dirent* create(int fd, char* path, short type, int mode);
void eSetTime(Dirent *entry, TimeSpec ts[2]);

#endif