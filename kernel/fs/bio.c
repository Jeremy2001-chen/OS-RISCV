// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
#include "defs.h"
#include "Type.h"
// #include "param.h"
#include "Sleeplock.h"
#include "Spinlock.h"
// #include "riscv.h"
// #include "defs.h"
// #include "fs.h"
#include "Driver.h"
#include "Sd.h"
#include "bio.h"
#include <FileSystem.h>

struct {
    struct Spinlock lock;
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // head.next is most recent, head.prev is least.
    struct buf head;
} bcache;

void binit(void) {
    struct buf* b;

    initLock(&bcache.lock, "bcache");

    // Create linked list of buffers
    bcache.head.prev = &bcache.head;
    bcache.head.next = &bcache.head;
    for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        initsleeplock(&b->lock, "buffer");
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf* bget(int dev, uint blockno) {
    struct buf* b;

    acquireLock(&bcache.lock);

    if (dev >= 0) {
        for (b = bcache.head.next; b != &bcache.head; b = b->next) {
            if (b->dev == dev && b->blockno == blockno) {
                b->refcnt++;
                releaseLock(&bcache.lock);
                acquiresleep(&b->lock);
                return b;
            }
        }
    }
    // Is the block already cached?    

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            releaseLock(&bcache.lock);
            acquiresleep(&b->lock);
            return b;
        }
    }
    panic("bget: no buffers");
}

struct buf* mountBlockRead(FileSystem* fs, u64 blockNum) {
    struct dirent* image = fs->image;
    FileSystem* parentFs = image->fileSystem;
    int parentBlockNum = getBlockNumber(image, blockNum); 
    if (parentBlockNum < 0) {
        return 0;
    }
    return parentFs->read(parentFs, parentBlockNum);
}

struct buf* blockRead(FileSystem* fs, u64 blockNum) {
    return bread(fs->deviceNumber, blockNum);
}

// Return a locked buf with the contents of the indicated block.
struct buf* bread(uint dev, uint blockno) {
    struct buf* b;
    b = bget(dev, blockno);
    if (!b->valid) {
        sdRead(b->data, b->blockno, 1);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf* b) {
    if (!holdingsleep(&b->lock))
        panic("bwrite");
    sdWrite(b->data, b->blockno, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf* b) {
    if (!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);

    acquireLock(&bcache.lock);
    b->refcnt--;
    if (b->refcnt == 0) {
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = bcache.head.next;
        b->prev = &bcache.head;
        bcache.head.next->prev = b;
        bcache.head.next = b;
    }

    releaseLock(&bcache.lock);
}

void bpin(struct buf* b) {
    acquireLock(&bcache.lock);
    b->refcnt++;
    releaseLock(&bcache.lock);
}

void bunpin(struct buf* b) {
    acquireLock(&bcache.lock);
    b->refcnt--;
    releaseLock(&bcache.lock);
}
