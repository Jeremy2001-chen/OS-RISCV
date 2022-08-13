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
static void eFreeInode(struct dirent *ep) {
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

static void eFindInode(struct dirent *entry, int pos) {
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

extern DirentCache direntCache;
//struct superblock fat;

/**
 * Read the Boot Parameter Block.
 * @return  0       if success
 *          -1      if fail
int fat32_init() {
#ifdef ZZY_DEBUG
    printf("[fat32_init] enter!\n");
#endif
    struct buf* b = bread(0, 0);
#ifdef ZZY_DEBUG
    printf("[fat32_init] bread finish!\n");
#endif
    if (strncmp((char const*)(b->data + 82), "FAT32", 5))
        panic("not FAT32 volume");
    // fat.bpb.byts_per_sec = *(uint16 *)(b->data + 11);
    memmove(&fat.bpb.byts_per_sec, b->data + 11,
            2);  // avoid misaligned load on k210
    fat.bpb.sec_per_clus = *(b->data + 13);
    fat.bpb.rsvd_sec_cnt = *(uint16*)(b->data + 14);
    fat.bpb.fat_cnt = *(b->data + 16);
    fat.bpb.hidd_sec = *(uint32*)(b->data + 28);
    fat.bpb.tot_sec = *(uint32*)(b->data + 32);
    fat.bpb.fat_sz = *(uint32*)(b->data + 36);
    fat.bpb.root_clus = *(uint32*)(b->data + 44);
    fat.first_data_sec =
        fat.bpb.rsvd_sec_cnt + fat.bpb.fat_cnt * fat.bpb.fat_sz;
    fat.data_sec_cnt = fat.bpb.tot_sec - fat.first_data_sec;
    fat.data_clus_cnt = fat.data_sec_cnt / fat.bpb.sec_per_clus;
    fat.byts_per_clus = fat.bpb.sec_per_clus * fat.bpb.byts_per_sec;
    brelse(b);

#ifdef ZZY_DEBUG
    printf("[FAT32 init]byts_per_sec: %d\n", fat.bpb.byts_per_sec);
    printf("[FAT32 init]root_clus: %d\n", fat.bpb.root_clus);
    printf("[FAT32 init]sec_per_clus: %d\n", fat.bpb.sec_per_clus);
    printf("[FAT32 init]fat_cnt: %d\n", fat.bpb.fat_cnt);
    printf("[FAT32 init]fat_sz: %d\n", fat.bpb.fat_sz);
    printf("[FAT32 init]first_data_sec: %d\n", fat.first_data_sec);
#endif

    // make sure that byts_per_sec has the same value with BSIZE
    if (BSIZE != fat.bpb.byts_per_sec)
        panic("byts_per_sec != BSIZE");
    initLock(&ecache.lock, "ecache");
    memset(&root, 0, sizeof(root));
    initsleeplock(&root.lock, "entry");
    root.attribute = (ATTR_DIRECTORY | ATTR_SYSTEM);
    root.first_clus = root.cur_clus = fat.bpb.root_clus;
    root.valid = 1;
    root.prev = &root;
    root.next = &root;
    root.filename[0]='/';
    for (struct dirent* de = ecache.entries;
         de < ecache.entries + ENTRY_CACHE_NUM; de++) {
        de->dev = 0;
        de->valid = 0;
        de->ref = 0;
        de->dirty = 0;
        de->parent = 0;
        de->next = root.next;
        de->prev = &root;
        initsleeplock(&de->lock, "entry");
        root.next->prev = de;
        root.next = de;
    }
    return 0;
}

 */
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
    clusterBitmap[cluster >> 6] &= ~(1 << (cluster & 63));
}

struct dirent* create(int fd, char* path, short type, int mode) {
    struct dirent *ep, *dp;
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
    
    elock(dp);
    if ((ep = ealloc(dp, name, mode)) == NULL) {
        eunlock(dp);
        eput(dp);
        return NULL;
    }

    
    if ((type == T_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
        (type == T_FILE && (ep->attribute & ATTR_DIRECTORY))) {
        eunlock(dp);
        eput(ep);
        eput(dp);
        return NULL;
    }

    eunlock(dp);
    eput(dp);

    elock(ep);
    
    return ep;
}

static uint rw_clus(FileSystem *fs, uint32 cluster,
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
static int reloc_clus(FileSystem *fs, struct dirent* entry, uint off, int alloc) {
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

int getBlockNumber(struct dirent* entry, int dataBlockNum) {
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
int eread(struct dirent* entry, int user_dst, u64 dst, uint off, uint n) {
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
int ewrite(struct dirent* entry, int user_src, u64 src, uint off, uint n) {
    if (entry->dev == NONE) {
        return n;
    }
    if (off > entry->file_size || off + n < off ||
        (u64)off + n > 0xffffffff || (entry->attribute & ATTR_READ_ONLY)) {
        return -1;
    }
    FileSystem *fs = entry->fileSystem;
    if (entry->first_clus ==
        0) {  // so file_size if 0 too, which requests off == 0
        entry->cur_clus = entry->first_clus = entry->inode.item[0] = alloc_clus(fs, entry->dev);
        entry->inodeMaxCluster = 1;
        entry->clus_cnt = 0;
        entry->dirty = 1;
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
            entry->dirty = 1;
        }
    }
    return tot;
}

// Returns a dirent struct. If name is given, check ecache. It is difficult to
// cache entries by their whole path. But when parsing a path, we open all the
// directories through it, which forms a linked list from the final file to the
// root. Thus, we use the "parent" pointer to recognize whether an entry with
// the "name" as given is really the file we want in the right path. Should
// never get root by eget, it's easy to understand.
static struct dirent* eget(struct dirent* parent, char* name) {
    struct dirent* ep;
    // acquireLock(&direntCache.lock);
    if (name) {
        for (int i = 0; i < ENTRY_CACHE_NUM; i++) {
            ep = &direntCache.entries[i];
            if (ep->valid == 1 && ep->parent == parent &&
                strncmp(ep->filename, name, FAT32_MAX_FILENAME) == 0) {
                if (ep->ref++ == 0) {
                    ep->parent->ref++;
                }
                // releaseLock(&direntCache.lock);
                return ep;
            }
        }
    }
    for (int i = 0; i < ENTRY_CACHE_NUM; i++) {
        ep = &direntCache.entries[i];
        if (ep->ref == 0) {
            ep->ref = 1;
            ep->dev = parent->dev;
            ep->off = 0;
            ep->valid = 0;
            ep->dirty = 0;
            ep->fileSystem = parent->fileSystem;
            eFreeInode(ep);
            // releaseLock(&direntCache.lock);
            return ep;
        }
    }
    panic("eget: insufficient ecache");
    return 0;
}

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

static void generate_shortname(char* shortname, char* name) {
    static char illegal[] = {
        '+', ',', ';', '=',
        '[', ']', 0};  // these are legal in l-n-e but not s-n-e
    int i = 0;
    char c, *p = name;
    for (int j = strlen(name) - 1; j >= 0; j--) {
        if (name[j] == '.') {
            p = name + j;
            break;
        }
    }
    while (i < CHAR_SHORT_NAME && (c = *name++)) {
        if (i == 8 && p) {
            if (p + 1 < name) {
                break;
            }  // no '.'
            else {
                name = p + 1, p = 0;
                continue;
            }
        }
        if (c == ' ') {
            continue;
        }
        if (c == '.') {
            if (name > p) {  // last '.'
                memset(shortname + i, ' ', 8 - i);
                i = 8, p = 0;
            }
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            c += 'A' - 'a';
        } else {
            if (strchr(illegal, c) != NULL) {
                c = '_';
            }
        }
        shortname[i++] = c;
    }
    while (i < CHAR_SHORT_NAME) {
        shortname[i++] = ' ';
    }
}

uint8 cal_checksum(uchar* shortname) {
    uint8 sum = 0;
    for (int i = CHAR_SHORT_NAME; i != 0; i--) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *shortname++;
    }
    return sum;
}

/**
 * Generate an on disk format entry and write to the disk. Caller must hold
 * dp->lock
 * @param   dp          the directory
 * @param   ep          entry to write on disk
 * @param   off         offset int the dp, should be calculated via dirlookup
 * before calling this
 */
void emake(struct dirent* dp, struct dirent* ep, uint off) {
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("emake: not dir");
    if (off % sizeof(union dentry))
        panic("emake: not aligned");

    assert(dp->fileSystem == ep->fileSystem);
    FileSystem *fs = ep->fileSystem;
    union dentry de;
    memset(&de, 0, sizeof(de));
    if (off <= 32) {
        if (off == 0) {
            strncpy(de.sne.name, ".          ", sizeof(de.sne.name));
        } else {
            strncpy(de.sne.name, "..         ", sizeof(de.sne.name));
        }
        de.sne.attr = ATTR_DIRECTORY;
        de.sne.fst_clus_hi =
            (uint16)(ep->first_clus >> 16);  // first clus high 16 bits
        de.sne.fst_clus_lo = (uint16)(ep->first_clus & 0xffff);  // low 16 bits
        de.sne.file_size = 0;  // filesize is updated in eupdate()
        de.sne._nt_res = ep->_nt_res;
        off = reloc_clus(fs, dp, off, 1);
        rw_clus(fs, dp->cur_clus, 1, 0, (u64)&de, off, sizeof(de));
    } else {
        int entcnt = (strlen(ep->filename) + CHAR_LONG_NAME - 1) /
                     CHAR_LONG_NAME;  // count of l-n-entries, rounds up
        char shortname[CHAR_SHORT_NAME + 1];
        memset(shortname, 0, sizeof(shortname));
        generate_shortname(shortname, ep->filename);
        de.lne.checksum = cal_checksum((uchar*)shortname);
        de.lne.attr = ATTR_LONG_NAME;
        for (int i = entcnt; i > 0; i--) {
            if ((de.lne.order = i) == entcnt) {
                de.lne.order |= LAST_LONG_ENTRY;
            }
            char* p = ep->filename + (i - 1) * CHAR_LONG_NAME;
            uint8* w = (uint8*)de.lne.name1;
            int end = 0;
            for (int j = 1; j <= CHAR_LONG_NAME; j++) {
                if (end) {
                    *w++ = 0xff;  // on k210, unaligned reading is illegal
                    *w++ = 0xff;
                } else {
                    if ((*w++ = *p++) == 0) {
                        end = 1;
                    }
                    *w++ = 0;
                }
                switch (j) {
                    case 5:
                        w = (uint8*)de.lne.name2;
                        break;
                    case 11:
                        w = (uint8*)de.lne.name3;
                        break;
                }
            }
            uint off2 = reloc_clus(fs, dp, off, 1);
            rw_clus(fs, dp->cur_clus, 1, 0, (u64)&de, off2, sizeof(de));
            off += sizeof(de);
        }
        memset(&de, 0, sizeof(de));
        strncpy(de.sne.name, shortname, sizeof(de.sne.name));
        de.sne.attr = ep->attribute;
        de.sne.fst_clus_hi =
            (uint16)(ep->first_clus >> 16);  // first clus high 16 bits
        de.sne.fst_clus_lo = (uint16)(ep->first_clus & 0xffff);  // low 16 bits
        de.sne.file_size = ep->file_size;  // filesize is updated in eupdate()
        de.sne._nt_res = ep->_nt_res;
        off = reloc_clus(fs, dp, off, 1);
        rw_clus(fs, dp->cur_clus, 1, 0, (u64)&de, off, sizeof(de));
    }
}

/**
 * Allocate an entry on disk. Caller must hold dp->lock.
 */
struct dirent* ealloc(struct dirent* dp, char* name, int attr) {
    if (!(dp->attribute & ATTR_DIRECTORY)) {
        panic("ealloc not dir");
    }
    
    if (dp->valid != 1 ||
        !(name = formatname(name))) {  // detect illegal character
        return NULL;
    }
    
    struct dirent* ep;
    uint off = 0;
    if ((ep = dirlookup(dp, name, &off)) != 0) {  // entry exists
        return ep;
    }
    
    ep = eget(dp, name);
    elock(ep);
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
    ep->parent = edup(dp);
    ep->off = off;
    ep->clus_cnt = 0;
    ep->cur_clus = 0;
    ep->dirty = 0;
    strncpy(ep->filename, name, FAT32_MAX_FILENAME);
    ep->filename[FAT32_MAX_FILENAME] = '\0';
    FileSystem *fs = ep->fileSystem;
    if (attr == ATTR_DIRECTORY) {  // generate "." and ".." for ep
        ep->attribute |= ATTR_DIRECTORY;
        ep->cur_clus = ep->first_clus = ep->inode.item[0] = alloc_clus(fs, dp->dev);
        ep->inodeMaxCluster = 1;
        emake(ep, ep, 0);
        emake(ep, dp, 32);
    } else {
        ep->attribute |= ATTR_ARCHIVE;
    }
    emake(dp, ep, off);
    ep->valid = 1;
    eunlock(ep);
    return ep;
}

struct dirent* edup(struct dirent* entry) {
    if (entry != 0) {
        // acquireLock(&direntCache.lock);
        entry->ref++;
        // releaseLock(&direntCache.lock);
    }
    
    return entry;
}

// Only update filesize and first cluster in this case.
// caller must hold entry->parent->lock
void eupdate(struct dirent* entry) {
    if (!entry->dirty || entry->valid != 1) {
        return;
    }
    uint entcnt = 0;
    FileSystem *fs = entry->fileSystem;
    uint32 off = reloc_clus(fs, entry->parent, entry->off, 0);
    rw_clus(fs, entry->parent->cur_clus, 0, 0, (u64)&entcnt, off, 1);
    entcnt &= ~LAST_LONG_ENTRY;
    off = reloc_clus(fs, entry->parent, entry->off + (entcnt << 5), 0);
    union dentry de;
    rw_clus(fs, entry->parent->cur_clus, 0, 0, (u64)&de, off, sizeof(de));
    de.sne.fst_clus_hi = (uint16)(entry->first_clus >> 16);
    de.sne.fst_clus_lo = (uint16)(entry->first_clus & 0xffff);
    de.sne.file_size = entry->file_size;
    de.sne._nt_res = entry->_nt_res;
    rw_clus(fs, entry->parent->cur_clus, 1, 0, (u64)&de, off, sizeof(de));
    entry->dirty = 0;
}

#define UTIME_NOW ((1l << 30) - 1l)
#define UTIME_OMIT ((1l << 30) - 2l)
void eSetTime(struct dirent *entry, TimeSpec ts[2]) {
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
void eremove(struct dirent* entry) {
    if (entry->valid != 1) {
        return;
    }
    FileSystem *fs = entry->fileSystem;
    uint entcnt = 0;
    uint32 off = entry->off;
    uint32 off2 = reloc_clus(fs, entry->parent, off, 0);
    rw_clus(fs, entry->parent->cur_clus, 0, 0, (u64)&entcnt, off2, 1);
    entcnt &= ~LAST_LONG_ENTRY;
    uint8 flag = EMPTY_ENTRY;
    for (int i = 0; i <= entcnt; i++) {
        rw_clus(fs, entry->parent->cur_clus, 1, 0, (u64)&flag, off2, 1);
        off += 32;
        off2 = reloc_clus(fs, entry->parent, off, 0);
    }
    entry->valid = -1;
}

// truncate a file
// caller must hold entry->lock*全部文件名目录项
void etrunc(struct dirent* entry) {
    FileSystem *fs = entry->fileSystem;

    for (uint32 clus = entry->first_clus; clus >= 2 && clus < FAT32_EOC;) {
        uint32 next = read_fat(fs, clus);
        free_clus(fs, clus);
        clus = next;
    }
    entry->file_size = 0;
    entry->first_clus = 0;
    eFreeInode(entry);
    entry->dirty = 1;
}

void elock(struct dirent* entry) {
    if (entry == 0 || entry->ref < 1)
        panic("elock");
    acquiresleep(&entry->lock);
}

void eunlock(struct dirent* entry) {
    if (entry == 0 || !holdingsleep(&entry->lock) || entry->ref < 1)
        panic("eunlock");
    releasesleep(&entry->lock);
}

void eput(struct dirent* entry) {
    return;
    // acquireLock(&direntCache.lock);
    MSG_PRINT("// acquireLock finish");
    if ((entry >= direntCache.entries && entry < direntCache.entries + ENTRY_CACHE_NUM) && entry->valid != 0 && entry->ref == 1) {
        // ref == 1 means no other process can have entry locked,
        // so this acquiresleep() won't block (or deadlock).
        acquiresleep(&entry->lock);
        MSG_PRINT("acquireSleep finish");
      //  entry->next->prev = entry->prev;
      //  entry->prev->next = entry->next;
      //  entry->next = root.next;
      //  entry->prev = &root;
      //  root.next->prev = entry;
      //  root.next = entry;
        // releaseLock(&direntCache.lock);
        if (entry->valid == -1) {  // this means some one has called eremove()
            etrunc(entry);
        } else {
            elock(entry->parent);
            eupdate(entry);
            eunlock(entry->parent);
        }
        releasesleep(&entry->lock);

        // Once entry->ref decreases down to 0, we can't guarantee the
        // entry->parent field remains unchanged. Because eget() may take the
        // entry away and write it.
        struct dirent* eparent = entry->parent;
        // acquireLock(&direntCache.lock);
        entry->ref--;
        // releaseLock(&direntCache.lock);
        if (entry->ref == 0) {
            eput(eparent);
        }
        return;
    }
    MSG_PRINT("end of eput");
    // entry->ref--;
    // releaseLock(&direntCache.lock);
}

//todo(need more)
void estat(struct dirent* ep, struct stat* st) {
    // strncpy(st->name, de->filename, STAT_MAX_NAME);
    // st->type = (de->attribute & ATTR_DIRECTORY) ? T_DIR : T_FILE;
    st->st_dev = ep->dev;
    st->st_size = ep->file_size;
    st->st_ino = (ep - direntCache.entries);
    st->st_mode = (ep->attribute & ATTR_DIRECTORY ? DIR_TYPE :
                ep->attribute & ATTR_CHARACTER_DEVICE ? CHR_TYPE : REG_TYPE);
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;  // What's this?
    st->st_blksize = ep->fileSystem->superBlock.bpb.byts_per_sec;
    st->st_blocks = st->st_size / st->st_blksize;
    // printf("attr: %d\n", st->st_mode);
    if (ep->parent != NULL) {
        uint entcnt = 0;
        FileSystem *fs = ep->fileSystem;
        uint32 off = reloc_clus(fs, ep->parent, ep->off, 0);
        rw_clus(fs, ep->parent->cur_clus, 0, 0, (u64)&entcnt, off, 1);
        entcnt &= ~LAST_LONG_ENTRY;
        off = reloc_clus(fs, ep->parent, ep->off + (entcnt << 5), 0);
        union dentry de;
        rw_clus(ep->fileSystem, ep->parent->cur_clus, 0, 0, (u64)&de, off, sizeof(de));
        st->st_atime_sec = (((u64)de.sne._crt_time_tenth) << 32) + 
            (((u64)de.sne._crt_time) << 16) + de.sne._crt_date;
        st->st_atime_nsec = 0;
        st->st_mtime_sec = (((u64)de.sne._lst_acce_date) << 32) + 
            (((u64)de.sne._lst_wrt_time) << 16) + de.sne._lst_wrt_date;
        st->st_mtime_nsec = 0;
        st->st_ctime_sec = 0;
        st->st_ctime_nsec = 0;
    } else {
        st->st_atime_sec = 0;
        st->st_atime_nsec = 0;
        st->st_mtime_sec = 0;
        st->st_mtime_nsec = 0;
        st->st_ctime_sec = 0;
        st->st_ctime_nsec = 0;
    }
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
static void read_entry_info(struct dirent* entry, union dentry* d) {
    entry->attribute = d->sne.attr;
    entry->first_clus = ((uint32)d->sne.fst_clus_hi << 16) | d->sne.fst_clus_lo;
    entry->inode.item[0] = entry->first_clus;
    entry->inodeMaxCluster = 1;
    entry->file_size = d->sne.file_size;
    entry->cur_clus = entry->first_clus;
    entry->clus_cnt = 0;
    entry->_nt_res = d->sne._nt_res;
}

/**
 * Read a directory from off, parse the next entry(ies) associated with one
 * file, or find empty entry slots. Caller must hold dp->lock.
 * @param   dp      the directory
 * @param   ep      the struct to be written with info
 * @param   off     offset off the directory
 * @param   count   to write the count of entries
 * @return  -1      meet the end of dir
 *          0       find empty slots
 *          1       find a file with all its entries
 */
int enext(struct dirent* dp, struct dirent* ep, uint off, int* count) {
    // assert(dp->fileSystem == ep->fileSystem);
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("enext not dir");
    if (ep->valid)
        panic("enext ep valid");
    if (off % 32)
        panic("enext not align");
    if (dp->valid != 1) {
        return -1;
    }

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

/**
 * Seacher for the entry in a directory and return a structure. Besides, record
 * the offset of some continuous empty slots that can fit the length of
 * filename. Caller must hold entry->lock.
 * @param   dp          entry of a directory file
 * @param   filename    target filename
 * @param   poff        offset of proper empty entry slots from the beginning of
 * the dir
 */
struct dirent* dirlookup(struct dirent* dp, char* filename, uint* poff) {
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
    
    if (dp->valid != 1) {
        return NULL;
    }
    
    struct dirent* ep = eget(dp, filename);
    if (ep->valid == 1) {
        return ep;
    }  // ecache hits

    int len = strlen(filename);
    int entcnt = (len + CHAR_LONG_NAME - 1) / CHAR_LONG_NAME +
                 1;  // count of l-n-entries, rounds up. plus s-n-e
    int count = 0;
    int type;
    uint off = 0;
    FileSystem *fs = dp->fileSystem;
    reloc_clus(fs, dp, 0, 0);
    while ((type = enext(dp, ep, off, &count) != -1)) {
        if (type == 0) {
            if (poff && count >= entcnt) {
                *poff = off;
                poff = 0;
            }
        } else if (strncmp(filename, ep->filename, FAT32_MAX_FILENAME) == 0) {
            ep->parent = edup(dp);
            ep->off = off;
            ep->valid = 1;
            return ep;
        }
        off += count << 5;
    }
    if (poff) {
        *poff = off;
    }
    eput(ep);
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

static struct dirent* jumpToLinkDirent(struct dirent* link) {
    char buf[FAT32_MAX_FILENAME];
    while (link && link->_nt_res == DT_LNK) {
        eread(link, 0, (u64)buf, 0, FAT32_MAX_FILENAME);
        link = ename(AT_FDCWD, buf, true);
    }
    assert(link != NULL);
    return link;
}

static struct dirent* lookup_path(int fd, char* path, int parent, char* name, bool jump) {
    struct dirent *entry, *next;
    
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
            struct dirent* mountDirent = &entry->head->root;
            eunlock(entry);
            elock(mountDirent);
    printf("%s %d\n", __FILE__, __LINE__);
            entry = edup(mountDirent);
        }

        if (parent && *path == '\0') {
            eunlock(entry);
            return entry;
        }
        if ((next = dirlookup(entry, name, 0)) == 0) {
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

struct dirent* ename(int fd, char* path, bool jump) {
    char name[FAT32_MAX_FILENAME + 1];
    return lookup_path(fd, path, 0, name, jump);
}

struct dirent* enameparent(int fd, char* path, char* name) {
    return lookup_path(fd, path, 1, name, true);
}
