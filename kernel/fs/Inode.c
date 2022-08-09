#include <Inode.h>
#include <Driver.h>
#include <string.h>

Inode inodes[INODE_NUM];
u64 inodeBitmap[INODE_NUM / 64];

int inodeAlloc() {
    for (int i = 0; i < sizeof(inodeBitmap); i++) {
        if (~inodeBitmap[i]) {
            int bit = LOW_BIT64(~inodeBitmap[i]);
            inodeBitmap[i] |= (1UL << bit);
            int ret = (i << 6) | bit;
            return ret;
        }
    }
    return -1;
}

void inodeFree(int x) {
    if (x == -1) {
        panic("");
    }
    assert(inodeBitmap[x >> 6] & (1UL << (x & 63)));
    inodeBitmap[x >> 6] &= ~(1UL << (x & 63));
}
