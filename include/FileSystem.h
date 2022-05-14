#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_
#include <Type.h>
#include <bio.h>
#include <fat.h>
#define MAX_NAME_LENGTH 64

struct buf;
typedef struct FileSystem {
    bool valid;
    char name[MAX_NAME_LENGTH];
    struct superblock superBlock;
    struct dirent root;
    struct dirent *image;
    FileSystem *next;
    int (*read)(struct buf **buf, u64 startSector, struct dirent *image);
} FileSystem;

typedef struct DirentCache {
    struct Spinlock lock;
    struct dirent entries[ENTRY_CACHE_NUM];
} DirentCache;

int fsAlloc(FileSystem **fs);
int fatInit(FileSystem *fs);
void initDirentCache();

#endif