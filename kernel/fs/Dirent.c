#include <Dirent.h>
#include <Driver.h>
#include <String.h>

Dirent dirents[DIRENT_NUM];
Dirent *direntHead;

int direntAlloc(Dirent **d) {
    assert(direntHead != NULL);
    *d = direntHead;
    direntHead = direntHead->nextBrother;
    return 0;
}

void direntFree(Dirent *d) {
    d->nextBrother = direntHead;
    direntHead = d;
}

void direntInit() {
    for (int i = 0; i < DIRENT_NUM; i++) {
        dirents[i].nextBrother = direntHead;
        direntHead = dirents + i;
    }
}
