#include "Sleeplock.h"
#include "Type.h"

#define BSIZE 512
#define NBUF 128

struct buf {
    int valid;  // has data been read from disk?
    int disk;   // does disk "own" buf?
    uint dev;
    uint blockno;
    struct Sleeplock lock;
    uint refcnt;
    struct buf* prev;  // LRU cache list
    struct buf* next;
    uchar data[BSIZE];
};

void binit(void);
struct buf* bread(uint dev, uint blockno);
void bwrite(struct buf* b);
void brelse(struct buf* b);
void bpin(struct buf* b);
void bunpin(struct buf* b);