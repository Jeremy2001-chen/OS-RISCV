#include <Defs.h>
#include "bio.h"
#include "fat.h"
#include "Sleeplock.h"
#include "Spinlock.h"
#include "stat.h"
#include "Type.h"
#include "string.h"
#include <Driver.h>
#include <Process.h>
#include <Debug.h>
#include <FileSystem.h>
#include <Sysfile.h>
#include <Thread.h>
#include <Riscv.h>

/* fields that start with "_" are something we don't use */

extern Inode inodes[];
static void eFreeInode(Dirent *ep) {
    for (int i = INODE_SECOND_ITEM_BASE; i < INODE_THIRD_ITEM_BASE; i++) {
        if (ep->inodeMaxCluster > INODE_SECOND_LEVEL_BOTTOM + (i - INODE_SECOND_ITEM_BASE) * INODE_ITEM_NUM) {
            inodeFree(ep->inode.item[i]);
            continue;
        }
        break;
    }
    for (int i = INODE_THIRD_ITEM_BASE; i < INODE_ITEM_NUM; i++) {
        int base = INODE_THIRD_LEVEL_BOTTOM + (i - INODE_SECOND_ITEM_BASE) * INODE_ITEM_NUM * INODE_ITEM_NUM;
        if (ep->inodeMaxCluster > base) {
            for (int j = 0; j < INODE_ITEM_NUM; j++) {
                if (ep->inodeMaxCluster > base + j * INODE_ITEM_NUM) {
                    inodeFree(inodes[ep->inode.item[i]].item[j]);
                    continue;
                }
                break;
            }
            inodeFree(ep->inode.item[i]);
            continue;
        }
        break;
    }
    ep->inodeMaxCluster = 0;
    return;
}

static void eFindInode(Dirent *entry, int pos) {
    assert(pos < entry->inodeMaxCluster);
    entry->clus_cnt = pos;
    if (pos < INODE_SECOND_LEVEL_BOTTOM) {
        entry->cur_clus = entry->inode.item[pos];
        return;
    }
    if (pos < INODE_THIRD_LEVEL_BOTTOM) {
        int idx = INODE_SECOND_ITEM_BASE + (pos - INODE_SECOND_LEVEL_BOTTOM) / INODE_ITEM_NUM;
        entry->cur_clus = inodes[entry->inode.item[idx]].item[(pos - INODE_SECOND_LEVEL_BOTTOM) % INODE_ITEM_NUM];
        return;
    }
    if (pos < INODE_THIRD_LEVEL_TOP) {
        int idx1 = INODE_THIRD_ITEM_BASE + (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM / INODE_ITEM_NUM;
        int idx2 = (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM % INODE_ITEM_NUM;
        entry->cur_clus = inodes[inodes[entry->inode.item[idx1]].item[idx2]].item[(pos - INODE_THIRD_LEVEL_BOTTOM) % INODE_ITEM_NUM];
        return;
    }
    panic("");
}

/**
 * @param   cluster   cluster number starts from 2, which means no 0 and 1
 */
static inline uint32 first_sec_of_clus(FileSystem *fs, uint32 cluster) {
    return ((cluster - 2) * fs->superBlock.bpb.sec_per_clus) + fs->superBlock.first_data_sec;
}

/**
 * For the given number of a data cluster, return the number of the sector in a
 * FAT table.
 * @param   cluster     number of a data cluster
 * @param   fat_num     number of FAT table from 1, shouldn't be larger than
 * bpb::fat_cnt
 */
static inline uint32  fat_sec_of_clus(FileSystem *fs, uint32 cluster, uint8 fat_num) {
    return fs->superBlock.bpb.rsvd_sec_cnt + (cluster << 2) / fs->superBlock.bpb.byts_per_sec +
           fs->superBlock.bpb.fat_sz * (fat_num - 1);
}

/**
 * For the given number of a data cluster, return the offest in the
 * corresponding sector in a FAT table.
 * @param   cluster   number of a data cluster
 */
static inline uint32 fat_offset_of_clus(FileSystem *fs, uint32 cluster) {
    return (cluster << 2) % fs->superBlock.bpb.byts_per_sec;
}

/**
 * Read the FAT table content corresponded to the given cluster number.
 * @param   cluster     the number of cluster which you want to read its content
 * in FAT table
 */
static uint32 read_fat(FileSystem *fs, uint32 cluster) {
    if (cluster >= FAT32_EOC) {
        return cluster;
    }
    if (cluster >
        fs->superBlock.data_clus_cnt + 1) {  // because cluster number starts at 2, not 0
        return 0;
    }
    uint32 fat_sec = fat_sec_of_clus(fs, cluster, 1);
    // here should be a cache layer for FAT table, but not implemented yet.
    struct buf* b = fs->read(fs, fat_sec);
    uint32 next_clus = *(uint32*)(b->data + fat_offset_of_clus(fs, cluster));
    brelse(b);
    return next_clus;
}

/**
 * Write the FAT region content corresponded to the given cluster number.
 * @param   cluster     the number of cluster to write its content in FAT table
 * @param   content     the content which should be the next cluster number of
 * FAT end of chain flag
 */
static int write_fat(FileSystem *fs, uint32 cluster, uint32 content) {
    if (cluster > fs->superBlock.data_clus_cnt + 1) {
        return -1;
    }
    uint32 fat_sec = fat_sec_of_clus(fs, cluster, 1);
    struct buf* b = fs->read(fs, fat_sec);
    uint off = fat_offset_of_clus(fs, cluster);
    *(uint32*)(b->data + off) = content;
    bwrite(b);
    brelse(b);
    return 0;
}

static void zero_clus(FileSystem *fs, uint32 cluster) {
    uint32 sec = first_sec_of_clus(fs, cluster);
    struct buf* b;
    for (int i = 0; i < fs->superBlock.bpb.sec_per_clus; i++) {
        b = fs->read(fs, sec++);
        memset(b->data, 0, BSIZE);
        bwrite(b);
        brelse(b);
    }
}

static uint32 alloc_clus(FileSystem *fs, uint8 dev) {
    u64 *clusterBitmap = (u64*)getFileSystemClusterBitmap(fs);
    int totalClusterNumber = fs->superBlock.bpb.fat_sz * fs->superBlock.bpb.byts_per_sec / sizeof(uint32);
    for (int i = 0; i < (totalClusterNumber / 64); i++) {
        if (~clusterBitmap[i]) {
            int bit = LOW_BIT64(~clusterBitmap[i]);
            clusterBitmap[i] |= (1UL << bit);
            int cluster = (i << 6) | bit;
            struct buf* b;
            uint32 const ent_per_sec = fs->superBlock.bpb.byts_per_sec / sizeof(uint32);
            uint32 sec = fs->superBlock.bpb.rsvd_sec_cnt + cluster / ent_per_sec;
            b = fs->read(fs, sec);
            int j = cluster % ent_per_sec;
            ((uint32*)(b->data))[j] = FAT32_EOC + 7;
            bwrite(b);
            brelse(b);
            zero_clus(fs, cluster);
            return cluster;
        }
    }
    panic("");
}

static void free_clus(FileSystem *fs, uint32 cluster) {
    write_fat(fs, cluster, 0);
    u64 *clusterBitmap = (u64*)getFileSystemClusterBitmap(fs);
    clusterBitmap[cluster >> 6] &= ~(1UL << (cluster & 63));
}

Dirent* create(int fd, char* path, short type, int mode) {
    Dirent *ep, *dp;
    char name[FAT32_MAX_FILENAME + 1];

    if ((dp = enameparent(fd, path, name)) == NULL) {
        return NULL;
    }
    
    //TODO 虚拟文件权限mode转fat格式的权限mode
    if (type == T_DIR) {
        mode = ATTR_DIRECTORY;
    }  else if (type == T_LINK) {
        mode = ATTR_LINK;
    } else if (type == T_CHAR) {
        mode = ATTR_CHARACTER_DEVICE;
    }  else if (mode & O_RDONLY) {
        mode = ATTR_READ_ONLY;
    } else {
        mode = 0;
    }
    
    if ((ep = ealloc(dp, name, mode)) == NULL) {
        return NULL;
    }
    
    if ((type == T_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
        (type == T_FILE && (ep->attribute & ATTR_DIRECTORY))) {
        return NULL;
    }
    
    return ep;
}

uint rw_clus(FileSystem *fs, uint32 cluster,
                    int write,
                    int user,
                    u64 data,
                    uint off,
                    uint n) {
    if (off + n > fs->superBlock.byts_per_clus)
        panic("offset out of range");
    // printf("%s %d: rw_clus get in\n", __FILE__, __LINE__);
    uint tot, m;
    struct buf* bp;
    uint sec = first_sec_of_clus(fs, cluster) + off / fs->superBlock.bpb.byts_per_sec;
    off = off % fs->superBlock.bpb.byts_per_sec;

    int bad = 0;
    for (tot = 0; tot < n; tot += m, off += m, data += m, sec++) {
        bp = fs->read(fs, sec);
        m = BSIZE - off % BSIZE;
        if (n - tot < m) {
            m = n - tot;
        }
        if (write) {
            if ((bad = either_copyin(bp->data + (off % BSIZE), user, data,
                                     m)) != -1) {
                bwrite(bp);
            }
        } else {
            bad = either_copyout(user, data, bp->data + (off % BSIZE), m);
        }
        brelse(bp);
        if (bad == -1) {
            break;
        }
    }
    // printf("%s %d: rw_clus get out\n", __FILE__, __LINE__);
    return tot;
}

/**
 * for the given entry, relocate the cur_clus field based on the off
 * @param   entry       modify its cur_clus field
 * @param   off         the offset from the beginning of the relative file
 * @param   alloc       whether alloc new cluster when meeting end of FAT chains
 * @return              the offset from the new cur_clus
 */
int reloc_clus(FileSystem *fs, Dirent* entry, uint off, int alloc) {
    assert(entry->first_clus != 0);
    assert(entry->inodeMaxCluster > 0);
    assert(entry->first_clus == entry->inode.item[0]);
    int clus_num = off / fs->superBlock.byts_per_clus;
    int ret = off % fs->superBlock.byts_per_clus;
    if (clus_num < entry->inodeMaxCluster) {
        eFindInode(entry, clus_num);
        return ret;
    }
    if (entry->inodeMaxCluster > 0) {
        eFindInode(entry, entry->inodeMaxCluster - 1);
    }
    while (clus_num > entry->clus_cnt) {
        int clus = read_fat(fs, entry->cur_clus);
        if (clus >= FAT32_EOC) {
            if (alloc) {
                clus = alloc_clus(fs, entry->dev);
                write_fat(fs, entry->cur_clus, clus);
            } else {
                entry->cur_clus = entry->first_clus;
                entry->clus_cnt = 0;
                return -1;
            }
        }
        entry->cur_clus = clus;
        entry->clus_cnt++;
        u32 pos = entry->clus_cnt;
        assert(pos == entry->inodeMaxCluster);
        entry->inodeMaxCluster++;
        if (pos < INODE_SECOND_LEVEL_BOTTOM) {
            entry->inode.item[pos] = clus;
            continue;
        }
        if (pos < INODE_THIRD_LEVEL_BOTTOM) {
            int idx1 = INODE_SECOND_ITEM_BASE + (pos - INODE_SECOND_LEVEL_BOTTOM) / INODE_ITEM_NUM;
            int idx2 = (pos - INODE_SECOND_LEVEL_BOTTOM) % INODE_ITEM_NUM;
            if (idx2 == 0) {
                entry->inode.item[idx1] = inodeAlloc();
            }
            inodes[entry->inode.item[idx1]].item[idx2] = clus;
            continue;
        }
        if (pos < INODE_THIRD_LEVEL_TOP) {
            int idx1 = INODE_THIRD_ITEM_BASE + (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM / INODE_ITEM_NUM;
            int idx2 = (pos - INODE_THIRD_LEVEL_BOTTOM) / INODE_ITEM_NUM % INODE_ITEM_NUM;
            int idx3 = (pos - INODE_THIRD_LEVEL_BOTTOM) % INODE_ITEM_NUM;
            if (idx3 == 0) {
                if (idx2 == 0) {
                    entry->inode.item[idx1] = inodeAlloc();
                }
                inodes[entry->inode.item[idx1]].item[idx2] = inodeAlloc();
            }
            inodes[inodes[entry->inode.item[idx1]].item[idx2]].item[idx3] = clus;
            continue;
        }
        panic("");
    }
    return ret;
}

int getBlockNumber(Dirent* entry, int dataBlockNum) {
    int offset = (dataBlockNum << 9);
    if (offset > entry->file_size) {
        return -1;
    }
    
    FileSystem *fs = entry->fileSystem;
    reloc_clus(fs, entry, offset, 0);
    
    return first_sec_of_clus(fs, entry->cur_clus) + offset % fs->superBlock.byts_per_clus / fs->superBlock.bpb.byts_per_sec;
}

/* like the original readi, but "reade" is odd, let alone "writee" */
// Caller must hold entry->lock.
int eread(Dirent* entry, int user_dst, u64 dst, uint off, uint n) {
    if (entry->dev == ZERO) {
        if (!either_memset(user_dst, dst, 0, n)) {
            return n;
        }
        panic("error!\n");
    } else if (entry->dev == OSRELEASE) {
        char osrelease[] = "10.2.0";
        if (!either_copyout(user_dst, dst, (char*)osrelease, sizeof(osrelease))) {
            return sizeof(osrelease);
        }
        panic("error!\n");
    }
    if (off > entry->file_size || off + n < off ||
        (entry->attribute & ATTR_DIRECTORY)) {
        return 0;
    }
    if (off + n > entry->file_size) {
        n = entry->file_size - off;
    }

    FileSystem *fs = entry->fileSystem;
    uint tot, m;
    for (tot = 0; entry->cur_clus < FAT32_EOC && tot < n;
         tot += m, off += m, dst += m) {
        reloc_clus(fs, entry, off, 0);
        m = fs->superBlock.byts_per_clus - off % fs->superBlock.byts_per_clus;
        if (n - tot < m) {
            m = n - tot;
        }
        if (rw_clus(fs, entry->cur_clus, 0, user_dst, dst, 
            off % fs->superBlock.byts_per_clus, m) != m) {
            break;
        }
    }
    return tot;
}

// Caller must hold entry->lock.
int ewrite(Dirent* entry, int user_src, u64 src, uint off, uint n) {
    if (entry->dev == NONE) {
        return n;
    }
    if (/*off > entry->file_size ||*/ off + n < off ||
        (u64)off + n > 0xffffffff || (entry->attribute & ATTR_READ_ONLY)) {
        return -1;
    }
    FileSystem *fs = entry->fileSystem;
    if (entry->first_clus ==
        0) {  // so file_size if 0 too, which requests off == 0
        entry->cur_clus = entry->first_clus = entry->inode.item[0] = alloc_clus(fs, entry->dev);
        entry->inodeMaxCluster = 1;
        entry->clus_cnt = 0;
    }
    uint tot, m;
    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        reloc_clus(fs, entry, off, 1);
        m = fs->superBlock.byts_per_clus - off % fs->superBlock.byts_per_clus;
        if (n - tot < m) {
            m = n - tot;
        }
        if (rw_clus(fs, entry->cur_clus, 1, user_src, src, 
            off % fs->superBlock.byts_per_clus, m) != m) {
            break;
        }
    }
    if (n > 0) {
        if (off > entry->file_size) {
            entry->file_size = off;
        }
    }
    return tot;
}

extern Dirent dirents[];

// trim ' ' in the head and tail, '.' in head, and test legality
char* formatname(char* name) {
    static char illegal[] = {'\"', '*', '/', ':', '<', '>', '?', '\\', '|', 0};
    char* p;
    while (*name == ' ' || *name == '.') {
        name++;
    }
    for (p = name; *p; p++) {
        char c = *p;
        if (c < 0x20 || strchr(illegal, c)) {
            return 0;
        }
    }
    while (p-- > name) {
        if (*p != ' ') {
            p[1] = '\0';
            break;
        }
    }
    return name;
}

uint8 cal_checksum(uchar* shortname) {
    uint8 sum = 0;
    for (int i = CHAR_SHORT_NAME; i != 0; i--) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *shortname++;
    }
    return sum;
}

/**
 * Allocate an entry on disk. Caller must hold dp->lock.
 */
Dirent* ealloc(Dirent* dp, char* name, int attr) {
    if (!(dp->attribute & ATTR_DIRECTORY)) {
        panic("ealloc not dir");
    }
    
    Dirent* ep;
    if ((ep = dirlookup(dp, name)) != 0) {  // entry exists
        return ep;
    }
    
    direntAlloc(&ep);
    if (attr == ATTR_LINK) {
        ep->attribute = 0;
        ep->_nt_res = DT_LNK;
    } else {
        ep->attribute = attr;
        ep->_nt_res = 0;
    }
    ep->file_size = 0;
    ep->first_clus = 0;
    eFreeInode(ep);
    ep->parent = dp;
    ep->nextBrother = dp->firstChild;
    dp->firstChild = ep;
    ep->clus_cnt = 0;
    ep->cur_clus = 0;
    ep->fileSystem = dp->fileSystem;
    strncpy(ep->filename, name, FAT32_MAX_FILENAME);
    ep->filename[FAT32_MAX_FILENAME] = '\0';
    FileSystem *fs = ep->fileSystem;
    if (attr == ATTR_DIRECTORY) {  // generate "." and ".." for ep
        ep->attribute |= ATTR_DIRECTORY;
        ep->cur_clus = ep->first_clus = ep->inode.item[0] = alloc_clus(fs, dp->dev);
        ep->inodeMaxCluster = 1;
    } else {
        ep->attribute |= ATTR_ARCHIVE;
    }
    return ep;
}

Dirent* edup(Dirent* entry) {    
    return entry;
}

#define UTIME_NOW ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
void eSetTime(Dirent *entry, TimeSpec ts[2]) {
    uint entcnt = 0;
    FileSystem *fs = entry->fileSystem;
    uint32 off = reloc_clus(fs, entry->parent, entry->off, 0);
    rw_clus(fs, entry->parent->cur_clus, 0, 0, (u64)&entcnt, off, 1);
    entcnt &= ~LAST_LONG_ENTRY;
    off = reloc_clus(fs, entry->parent, entry->off + (entcnt << 5), 0);
    union dentry de;
    rw_clus(entry->fileSystem, entry->parent->cur_clus, 0, 0, (u64)&de, off, sizeof(de));
    u64 time = r_time();
    TimeSpec now;
    now.second = time / 1000000;
    now.nanoSecond = time % 1000000 * 1000;
    if (ts[0].nanoSecond != UTIME_OMIT) {
        if (ts[0].nanoSecond == UTIME_NOW) {
            ts[0].second = now.second;
        }
        de.sne._crt_date = ts[0].second & ((1 << 16) - 1);
        de.sne._crt_time = (ts[0].second >> 16) & ((1 << 16) - 1);
        de.sne._crt_time_tenth = (ts[0].second >> 32) & ((1 << 8) - 1);
    }
    if (ts[1].nanoSecond != UTIME_OMIT) {
        if (ts[1].nanoSecond == UTIME_NOW) {
            ts[1].second = now.second;
        }
        de.sne._lst_wrt_date = ts[1].second & ((1 << 16) - 1);
        de.sne._lst_wrt_time = (ts[1].second >> 16) & ((1 << 16) - 1);
        de.sne._lst_acce_date = (ts[1].second >> 32) & ((1 << 16) - 1);
    }
    rw_clus(entry->fileSystem, entry->parent->cur_clus, true, 0, (u64)&de, off, sizeof(de));
}

// caller must hold entry->lock
// caller must hold entry->parent->lock
// remove the entry in its parent directory
void eremove(Dirent* entry) {
    Dirent *i = entry->parent->firstChild;
    if (i == entry) {
        entry->parent->firstChild = entry->nextBrother;
    } else {
        for (; i->nextBrother; i = i->nextBrother) {
            if (i->nextBrother == entry) {
                i->nextBrother = entry->nextBrother;
                break;
            }
        }
    }
    direntFree(entry);
    for(int i = 1; i <= 300; i++) {
        printf("");
    }
}

// truncate a file
// caller must hold entry->lock*全部文件名目录项
void etrunc(Dirent* entry) {
    FileSystem *fs = entry->fileSystem;

    for (uint32 clus = entry->first_clus; clus >= 2 && clus < FAT32_EOC;) {
        uint32 next = read_fat(fs, clus);
        free_clus(fs, clus);
        clus = next;
    }
    entry->file_size = 0;
    entry->first_clus = 0;
    eFreeInode(entry);
}

void elock(Dirent* entry) {
}

void eunlock(Dirent* entry) {
}

void eput(Dirent* entry) {
}

//todo(need more)
void estat(Dirent* ep, struct stat* st) {
    // strncpy(st->name, de->filename, STAT_MAX_NAME);
    // st->type = (de->attribute & ATTR_DIRECTORY) ? T_DIR : T_FILE;
    st->st_dev = ep->dev;
    st->st_size = ep->file_size;
    st->st_ino = (ep - dirents);
    st->st_mode = (ep->attribute & ATTR_DIRECTORY ? DIR_TYPE :
                ep->attribute & ATTR_CHARACTER_DEVICE ? CHR_TYPE : REG_TYPE);
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;  // What's this?
    st->st_blksize = ep->fileSystem->superBlock.bpb.byts_per_sec;
    st->st_blocks = st->st_size / st->st_blksize;
    // printf("attr: %d\n", st->st_mode);
    // if (ep->parent != NULL) {
    //     uint entcnt = 0;
    //     FileSystem *fs = ep->fileSystem;
    //     uint32 off = reloc_clus(fs, ep->parent, ep->off, 0);
    //     rw_clus(fs, ep->parent->cur_clus, 0, 0, (u64)&entcnt, off, 1);
    //     entcnt &= ~LAST_LONG_ENTRY;
    //     off = reloc_clus(fs, ep->parent, ep->off + (entcnt << 5), 0);
    //     union dentry de;
    //     rw_clus(ep->fileSystem, ep->parent->cur_clus, 0, 0, (u64)&de, off, sizeof(de));
    //     st->st_atime_sec = (((u64)de.sne._crt_time_tenth) << 32) + 
    //         (((u64)de.sne._crt_time) << 16) + de.sne._crt_date;
    //     st->st_atime_nsec = 0;
    //     st->st_mtime_sec = (((u64)de.sne._lst_acce_date) << 32) + 
    //         (((u64)de.sne._lst_wrt_time) << 16) + de.sne._lst_wrt_date;
    //     st->st_mtime_nsec = 0;
    //     st->st_ctime_sec = 0;
    //     st->st_ctime_nsec = 0;
    // } else {
    //     st->st_atime_sec = 0;
    //     st->st_atime_nsec = 0;
    //     st->st_mtime_sec = 0;
    //     st->st_mtime_nsec = 0;
    //     st->st_ctime_sec = 0;
    //     st->st_ctime_nsec = 0;
    // }
}

/**
 * Seacher for the entry in a directory and return a structure. Besides, record
 * the offset of some continuous empty slots that can fit the length of
 * filename. Caller must hold entry->lock.
 * @param   dp          entry of a directory file
 * @param   filename    target filename
 * @param   poff        offset of proper empty entry slots from the beginning of
 * the dir
 */
Dirent* dirlookup(Dirent* dp, char* filename) {
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("dirlookup not DIR");

    if (strncmp(filename, ".", FAT32_MAX_FILENAME) == 0) {
        return edup(dp);
    } else if (strncmp(filename, "..", FAT32_MAX_FILENAME) == 0) {
        if (dp == &dp->fileSystem->root) {
            return edup(&dp->fileSystem->root);
        }
        return edup(dp->parent);
    }
    
    for (Dirent *ep = dp->firstChild; ep; ep = ep->nextBrother) {
        if (strncmp(ep->filename, filename, FAT32_MAX_FILENAME) == 0) {
            return ep;
        }
    }
    return NULL;
}

static char* skipelem(char* path, char* name) {
    while (*path == '/') {
        path++;
    }
    if (*path == 0) {
        return NULL;
    }
    char* s = path;
    while (*path != '/' && *path != 0) {
        path++;
    }
    int len = path - s;
    if (len > FAT32_MAX_FILENAME) {
        len = FAT32_MAX_FILENAME;
    }
    name[len] = 0;
    memmove(name, s, len);
    while (*path == '/') {
        path++;
    }
    return path;
}

static Dirent* jumpToLinkDirent(Dirent* link) {
    char buf[FAT32_MAX_FILENAME];
    while (link && link->_nt_res == DT_LNK) {
        eread(link, 0, (u64)buf, 0, FAT32_MAX_FILENAME);
        link = ename(AT_FDCWD, buf, true);
    }
    assert(link != NULL);
    return link;
}

static Dirent* lookup_path(int fd, char* path, int parent, char* name, bool jump) {
    Dirent *entry, *next;
    
    if (*path != '/' && fd != AT_FDCWD && fd >= 0 && fd < NOFILE) {
        if (myProcess()->ofile[fd] == 0) {
            return NULL;
        }
        entry = edup(myProcess()->ofile[fd]->ep);
    } else if (*path == '/') {
        extern FileSystem *rootFileSystem;
        entry = edup(&rootFileSystem->root);
    } else if (*path != '\0' && fd == AT_FDCWD) {
        entry = edup(myProcess()->cwd);
    } else {
        return NULL;
    }
    
    while ((path = skipelem(path, name)) != 0) {
        entry = jumpToLinkDirent(entry);
        elock(entry);
        if (!(entry->attribute & ATTR_DIRECTORY)) {
            eunlock(entry);
            eput(entry);
            return NULL;
        }
        
        if (entry->head != NULL) {
            Dirent* mountDirent = &entry->head->root;
            eunlock(entry);
            elock(mountDirent);
            entry = edup(mountDirent);
        }

        if (parent && *path == '\0') {
            eunlock(entry);
            return entry;
        }
        if ((next = dirlookup(entry, name)) == 0) {
            eunlock(entry);
            eput(entry);
            return NULL;
        }

        eunlock(entry);
        eput(entry);
        entry = next;

    }

    if (jump) {
        entry = jumpToLinkDirent(entry);
    }

    if (parent) {
        eput(entry);
        return NULL;
    }
    return entry;
}

Dirent* ename(int fd, char* path, bool jump) {
    char name[FAT32_MAX_FILENAME + 1];
    return lookup_path(fd, path, 0, name, jump);
}

Dirent* enameparent(int fd, char* path, char* name) {
    return lookup_path(fd, path, 1, name, true);
}
