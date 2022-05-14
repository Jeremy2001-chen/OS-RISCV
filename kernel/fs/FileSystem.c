#include "FileSystem.h"
#include <bio.h>
#include <Queue.h>
#include <string.h>
#include <Driver.h>
FileSystem fileSystem[32];

int fsAlloc(FileSystem **fs) {
    for (int i = 0; i < 32; i++) {
        if (!fileSystem[i].valid) {
            *fs = &fileSystem[i];
            fileSystem[i].valid = true;
            return 0;
        }
    }
    return -1;
}

DirentCache direntCache;
// fs's read, name, mount_point should be inited
int fatInit(FileSystem *fs) {
    printf("[FAT32 init]fat init begin\n");
    struct buf *b;
    if (fs->read(&b, 0, fs->image) < 0) {
        panic("");
    }
    if (strncmp((char const*)(b->data + 82), "FAT32", 5)) {
        panic("not FAT32 volume");
        return -1;
    }
    memmove(&fs->superBlock.bpb.byts_per_sec, b->data + 11, 2); 
    fs->superBlock.bpb.sec_per_clus = *(b->data + 13);
    fs->superBlock.bpb.rsvd_sec_cnt = *(uint16*)(b->data + 14);
    fs->superBlock.bpb.fat_cnt = *(b->data + 16);
    fs->superBlock.bpb.hidd_sec = *(uint32*)(b->data + 28);
    fs->superBlock.bpb.tot_sec = *(uint32*)(b->data + 32);
    fs->superBlock.bpb.fat_sz = *(uint32*)(b->data + 36);
    fs->superBlock.bpb.root_clus = *(uint32*)(b->data + 44);
    fs->superBlock.first_data_sec = fs->superBlock.bpb.rsvd_sec_cnt + fs->superBlock.bpb.fat_cnt * fs->superBlock.bpb.fat_sz;
    fs->superBlock.data_sec_cnt = fs->superBlock.bpb.tot_sec - fs->superBlock.first_data_sec;
    fs->superBlock.data_clus_cnt = fs->superBlock.data_sec_cnt / fs->superBlock.bpb.sec_per_clus;
    fs->superBlock.byts_per_clus = fs->superBlock.bpb.sec_per_clus * fs->superBlock.bpb.byts_per_sec;
    brelse(b);

    printf("[FAT32 init]read fat s\n");
#ifdef ZZY_DEBUG
    printf("[FAT32 init]byts_per_sec: %d\n", fat.bpb.byts_per_sec);
    printf("[FAT32 init]root_clus: %d\n", fat.bpb.root_clus);
    printf("[FAT32 init]sec_per_clus: %d\n", fat.bpb.sec_per_clus);
    printf("[FAT32 init]fat_cnt: %d\n", fat.bpb.fat_cnt);
    printf("[FAT32 init]fat_sz: %d\n", fat.bpb.fat_sz);
    printf("[FAT32 init]first_data_sec: %d\n", fat.first_data_sec);
#endif

    // make sure that byts_per_sec has the same value with BSIZE
    if (BSIZE != fs->superBlock.bpb.byts_per_sec)
        panic("byts_per_sec != BSIZE");
    memset(&fs->root, 0, sizeof(fs->root));
    initsleeplock(&fs->root.lock, "entry");
    fs->root.attribute = (ATTR_DIRECTORY | ATTR_SYSTEM);
    fs->root.first_clus = fs->root.cur_clus = fs->superBlock.bpb.root_clus;
    fs->root.valid = 1;
    fs->root.filename[0]='/';
    
    printf("[FAT32 init]fat init end\n");
    return 0;
}

FileSystem rootFileSystem;
void initDirentCache() {
    rootFileSystem.root.fileSystem = &rootFileSystem;
    initLock(&direntCache.lock, "ecache");
   // fs->root.prev = &fs->root;
   // fs->root.next = &fs->root;
    for (struct dirent* de = direntCache.entries;
         de < direntCache.entries + ENTRY_CACHE_NUM; de++) {
        de->dev = 0;
        de->valid = 0;
        de->ref = 0;
        de->dirty = 0;
        de->parent = 0;
     //   de->next = fs->root.next;
     //   de->prev = &fs->root;
        initsleeplock(&de->lock, "entry");
     //   fs->root.next->prev = de;
     //   fs->root.next = de;
    }
}