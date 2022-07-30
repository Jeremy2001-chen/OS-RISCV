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
    struct File *image;
    FileSystem *next;
    int deviceNumber;
    struct buf* (*read)(struct FileSystem *fs, u64 blockNum);
} FileSystem;

typedef struct DirentCache {
    struct Spinlock lock;
    struct dirent entries[ENTRY_CACHE_NUM];
} DirentCache;

typedef struct FileSystemStatus {
	unsigned long f_type, f_bsize;
	u64 f_blocks, f_bfree, f_bavail;
	u64 f_files, f_ffree;
	u64 f_fsid;
	unsigned long f_namelen, f_frsize, f_flags, f_spare[4];
} FileSystemStatus;

int fsAlloc(FileSystem **fs);
int fatInit(FileSystem *fs);
void initDirentCache();
int getFsStatus(char *path, FileSystemStatus *fss);

#endif