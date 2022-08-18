#include <Dirent.h>
#include <Driver.h>
#include <string.h>

Dirent dirents[DIRENT_NUM];
u64 direntBitmap[DIRENT_NUM / 64];

int direntAlloc(Dirent **d) {
    for (int i = 0; i < DIRENT_NUM / 64; i++) {
        if (~direntBitmap[i]) {
            int bit = LOW_BIT64(~direntBitmap[i]);
            direntBitmap[i] |= (1UL << bit);
            *d = &dirents[(i << 6) | bit];
            memset(*d, 0, sizeof(Dirent));
            return 0;
        }
    }
    panic("");
}

void direntFree(Dirent *d) {
    u32 num = d - dirents;
    direntBitmap[num >> 6] &= ~(1UL << (num & 63));
}
