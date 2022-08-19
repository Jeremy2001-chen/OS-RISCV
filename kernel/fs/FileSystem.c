#include "FileSystem.h"
#include <bio.h>
#include <Queue.h>
#include <string.h>
#include <Driver.h>
#include <file.h>
#include <Sysfile.h>
#include <Page.h>
#include <Dirent.h>
FileSystem fileSystem[32];

int fsAlloc(FileSystem **fs) {
    for (int i = 0; i < 32; i++) {
        if (!fileSystem[i].valid) {
            *fs = &fileSystem[i];
            memset(*fs, 0, sizeof(FileSystem));
            fileSystem[i].valid = true;
            return 0;
        }
    }
    return -1;
}

/**
 * Read filename from directory entry.
 * @param   buffer      pointer to the array that stores the name
 * @param   raw_entry   pointer to the entry in a sector buffer
 * @param   islong      if non-zero, read as l-n-e, otherwise s-n-e.
 */
static void read_entry_name(char* buffer, union dentry* d) {
    if (d->lne.attr == ATTR_LONG_NAME) {  // long entry branch
        wchar temp[NELEM(d->lne.name1)];
        memmove(temp, d->lne.name1, sizeof(temp));
        snstr(buffer, temp, NELEM(d->lne.name1));
        buffer += NELEM(d->lne.name1);
        snstr(buffer, d->lne.name2, NELEM(d->lne.name2));
        buffer += NELEM(d->lne.name2);
        snstr(buffer, d->lne.name3, NELEM(d->lne.name3));
    } else {
        // assert: only "." and ".." will enter this branch
        memset(buffer, 0, CHAR_SHORT_NAME + 2);  // plus '.' and '\0'
        int i;
        for (i = 0; d->sne.name[i] != ' ' && i < 8; i++) {
            buffer[i] = d->sne.name[i];
        }
        if (d->sne.name[8] != ' ') {
            buffer[i++] = '.';
        }
        for (int j = 8; j < CHAR_SHORT_NAME; j++, i++) {
            if (d->sne.name[j] == ' ') {
                break;
            }
            buffer[i] = d->sne.name[j];
        }
    }
}

/**
 * Read entry_info from directory entry.
 * @param   entry       pointer to the structure that stores the entry info
 * @param   raw_entry   pointer to the entry in a sector buffer
 */
static void read_entry_info(Dirent* entry, union dentry* d) {
    entry->attribute = d->sne.attr;
    entry->first_clus = ((uint32)d->sne.fst_clus_hi << 16) | d->sne.fst_clus_lo;
    entry->inode.item[0] = entry->first_clus;
    entry->inodeMaxCluster = 1;
    entry->file_size = d->sne.file_size;
    entry->cur_clus = entry->first_clus;
    entry->clus_cnt = 0;
    entry->_nt_res = d->sne._nt_res;
}

int reloc_clus(FileSystem *fs, Dirent* entry, uint off, int alloc);
uint rw_clus(FileSystem *fs, uint32 cluster,
                    int write,
                    int user,
                    u64 data,
                    uint off,
                    uint n);
static int enext(Dirent* dp, Dirent* ep, uint off, int* count) {
    // assert(dp->fileSystem == ep->fileSystem);
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("enext not dir");
    if (off % 32)
        panic("enext not align");

    union dentry de;
    int cnt = 0;
    memset(ep->filename, 0, FAT32_MAX_FILENAME + 1);
    FileSystem *fs = dp->fileSystem;
    for (int off2; (off2 = reloc_clus(fs, dp, off, 0)) != -1; off += 32) {
        if (rw_clus(fs, dp->cur_clus, 0, 0, (u64)&de, off2, 32) != 32 ||
            de.lne.order == END_OF_ENTRY) {
            return -1;
        }
        if (de.lne.order == EMPTY_ENTRY) {
            cnt++;
            continue;
        } else if (cnt) {
            *count = cnt;
            return 0;
        }
        if (de.lne.attr == ATTR_LONG_NAME) {
            int lcnt = de.lne.order & ~LAST_LONG_ENTRY;
            if (de.lne.order & LAST_LONG_ENTRY) {
                *count = lcnt + 1;  // plus the s-n-e;
                count = 0;
            }
            read_entry_name(ep->filename + (lcnt - 1) * CHAR_LONG_NAME, &de);
        } else {
            if (count) {
                *count = 1;
                read_entry_name(ep->filename, &de);
            }
            read_entry_info(ep, &de);
            return 1;
        }
    }
    return -1;
}

void loadDirents(FileSystem *fs, Dirent *parent) {
    u32 off = 0;
    int type;
    reloc_clus(fs, parent, 0, 0);
    Dirent *ep;
    direntAlloc(&ep);
    int count;
    while ((type = enext(parent, ep, off, &count) != -1)) {
        if (type == 0) {
            continue;
        }
        ep->parent = parent;
        ep->off = off;
        ep->nextBrother = parent->firstChild;
        ep->fileSystem = fs;
        parent->firstChild = ep;
        printf("name: %s, parent: %s\n", ep->filename, parent->filename);
        if ((ep->attribute & ATTR_DIRECTORY) && (off > 32 || parent == &fs->root)) {
            loadDirents(fs, ep);
        }
        direntAlloc(&ep);
        off += count << 5;
    }
    direntFree(ep);
}

FileSystem *rootFileSystem;
// fs's read, name, mount_point should be inited
int fatInit(FileSystem *fs) {
    printf("[FAT32 init]fat init begin\n");
    struct buf *b = fs->read(fs, 0);
    if (b == 0) {
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

    // printf("[FAT32 init]read fat s\n");
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
    fs->root.attribute = (ATTR_DIRECTORY | ATTR_SYSTEM);
    memset(&fs->root.inode, -1, sizeof(Inode));
    fs->root.inode.item[0] = fs->root.first_clus = fs->root.cur_clus = fs->superBlock.bpb.root_clus;
    fs->root.inodeMaxCluster = 1;
    fs->root.filename[0]='/';
    fs->root.fileSystem = fs;
    // fs->superBlock.bpb.fat_sz = MIN(512, fs->superBlock.bpb.fat_sz);
    int totalClusterNumber = fs->superBlock.bpb.fat_sz * fs->superBlock.bpb.byts_per_sec / sizeof(uint32);
    u64 *clusterBitmap = (u64*)getFileSystemClusterBitmap(fs);
    int cnt = 0;
        extern u64 kernelPageDirectory[];
    do {
        PhysicalPage *pp;
        if (pageAlloc(&pp) < 0) {
            panic("");
        }
        if (pageInsert(kernelPageDirectory, ((u64)clusterBitmap) + cnt, page2pa(pp), PTE_READ | PTE_WRITE) < 0) {
            panic("");
        }
        cnt += PAGE_SIZE;
    } while (cnt * 8 < totalClusterNumber);
    uint32 sec = fs->superBlock.bpb.rsvd_sec_cnt;
    uint32 const ent_per_sec = fs->superBlock.bpb.byts_per_sec / sizeof(uint32);
    for (uint32 i = 0; i < fs->superBlock.bpb.fat_sz; i++, sec++) {
        b = fs->read(fs, sec);
        for (uint32 j = 0; j < ent_per_sec; j++) {
            if (((uint32*)(b->data))[j]) {
                int no = i * ent_per_sec + j;
                clusterBitmap[no >> 6] |= (1UL << (no & 63));
            }
        }
        brelse(b);
    }
    loadDirents(fs, &fs->root);
    
    printf("[FAT32 init]fat init end\n");
    return 0;
}

void initRootFileSystem() {
    struct File* file = filealloc();
    rootFileSystem->image = file;
    file->type = FD_DEVICE;
    file->major = 0;
    file->readable = true;
    file->writable = true;
}

int getFsStatus(char *path, FileSystemStatus *fss) {
    Dirent *de;
    if ((de = ename(AT_FDCWD, path, true)) == NULL) {
        return -1;
    }
    FileSystem *fs = de->fileSystem;
    fss->f_bsize = 189;
    fss->f_blocks = fs->superBlock.bpb.tot_sec - fs->superBlock.first_data_sec;
    fss->f_bfree = 1;
    fss->f_bavail = 2;
    fss->f_files = 4;
    fss->f_ffree = 3;
    fss->f_namelen = FAT32_MAX_FILENAME;
    return 0;
}
